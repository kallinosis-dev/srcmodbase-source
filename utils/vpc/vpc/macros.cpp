//========= Copyright � 1996-2016, Valve Corporation, All rights reserved. ============//

#include <ranges>
#include "tier1/splitstring.h"

#include "macros.h"

#include "vpc.h"

inline bool IsValidMacroNameChar( char ch )
{
    return ch == '_' || V_isalnum( ch );
}

CMacro::CMacro( const char *pMacroName, const char *pMacroValue, const char *pConfigurationName, bool bSystemMacro, bool bSetupDefine )
{
	SetMacroName( pMacroName );
	m_Value = pMacroValue;

	if ( pConfigurationName )
	{
		// property macros (i.e. with configurations) are purposely narrow
		// they are not interchangeable with non-property macros that provide a different set of features
		// this is just to ensure that hacks don't come along with a misunderstanding
		Assert( bSystemMacro == false );
		Assert( bSetupDefine == false );
		
		if ( !pConfigurationName[0] )
		{
			// valid configuration name is mandatory
			g_pVPC->VPCError( "Missing expected configuration for property macro '%s'.", pMacroName );
		}

		m_ConfigurationName = pConfigurationName;

		m_bSetupDefineInProjectFile = false;
		m_bSystemMacro = false;
	}
	else
	{
		m_bSetupDefineInProjectFile = bSetupDefine;
		m_bSystemMacro = bSystemMacro;
	}

	m_pFNResolveDynamicMacro = nullptr;
}

CMacro::CMacro( const char *pMacroName, MacroResolveFn pFNResolveValue )
{
	SetMacroName( pMacroName );
	m_pFNResolveDynamicMacro = pFNResolveValue;
	m_bSetupDefineInProjectFile = false;
	m_bSystemMacro = true;
}

void CMacro::SetMacroName( const char *pMacroName )
{
	m_nBaseNameLength = V_strlen( pMacroName );
	if ( m_nBaseNameLength >= MAX_MACRO_NAME )
	{
		g_pVPC->VPCError( "Macro name '%s' too long.", pMacroName );
	}

	for ( int i = 0; i < m_nBaseNameLength; i++ )
	{
		if ( !IsValidMacroNameChar( pMacroName[i] ) )
		{
			g_pVPC->VPCError( "Macro name '%s' contains illegal character '%c'.",
								pMacroName, pMacroName[i] );
		}
	}
    
	// Internally adds another one for terminator.
	m_FullName.SetLength( m_nBaseNameLength + 1 );
	char *pFullName = m_FullName.Access();
	*pFullName = '$';
	V_strcpy( pFullName + 1, pMacroName );
}



//-----------------------------------------------------------------------------
// System macros are created by VPC and are expected to persist across projects.
/// They appear as Read Only to scripts.
//-----------------------------------------------------------------------------
CMacro * CMacroStorage::SetAsSystem( const char *pMacroName, const char *pMacroValue, bool bSetupDefineInProjectFile )
{
	g_pVPC->VPCStatus( false, "Set System Macro: $%s = %s", pMacroName, pMacroValue );

	CMacro *pMacro = Get( pMacroName );
	if ( !pMacro )
	{
		// create a system type macro
		pMacro = new CMacro( pMacroName, pMacroValue, nullptr, true, bSetupDefineInProjectFile );
		_macros.InsertWithDupes( pMacroName, pMacro );
		return pMacro;
	}


	// found existing macro
	if ( pMacro->IsPropertyMacro() )
	{
		// duplicate macro names not allowed
		g_pVPC->VPCError( "Macro '%s' already defined as a property macro.", pMacro->GetName() );
	}

	if ( !pMacro->IsSystemMacro() )
	{
		// internal macros cannot clash with script macros
		g_pVPC->VPCError( "$Macro '%s' already defined by script.", pMacro->GetName() );
	}

	// update value
	pMacro->SetValue( pMacroValue );

	return pMacro;
}

CMacro * CMacroStorage::SetAsDynamic( const char *pMacroName, MacroResolveFn pFNResolveValue)
{
	g_pVPC->VPCStatus( false, "Set Dynamic Macro: $%s", pMacroName );

	CMacro *pMacro = Get( pMacroName );
	if ( !pMacro )
	{
		// create a system type macro
		pMacro = new CMacro( pMacroName, pFNResolveValue );
		_macros.InsertWithDupes( pMacroName, pMacro );
		return pMacro;
	}


	// found existing macro
	if ( pMacro->IsPropertyMacro() )
	{
		// duplicate macro names not allowed
		g_pVPC->VPCError( "Macro '%s' already defined as a property macro.", pMacro->GetName() );
	}

	if ( !pMacro->IsSystemMacro() )
	{
		// internal macros cannot clash with script macros
		g_pVPC->VPCError( "$Macro '%s' already defined by script.", pMacro->GetName() );
	}

	// update value
	pMacro->SetResolveFunc( pFNResolveValue );

	return pMacro;
}

//-----------------------------------------------------------------------------
// Script macros are created by a project script based on THEIR state. They are removed at the conclusion of that project
// to avoid polluting the next project that gets processed.
//-----------------------------------------------------------------------------
CMacro * CMacroStorage::SetAsScript( const char *pMacroName, const char *pMacroValue, bool bSetupDefineInProjectFile )
{
	g_pVPC->VPCStatus( false, "Set Script Macro: $%s = %s", pMacroName, pMacroValue );

	CMacro *pMacro = Get( pMacroName );
	if ( pMacro )
	{
		// found existing macro
		if ( pMacro->IsPropertyMacro() )
		{
			// duplicate macro names not allowed
			g_pVPC->VPCError( "Macro '%s' already defined as a property macro.", pMacro->GetName() );
		}
		/*
		if ( pMacro->IsSystemMacro() )
		{
			// scripts are not allowed to alter system macros
			g_pVPC->VPCError( "Script not allowed to alter system macro '%s'.", pMacro->GetName() );
		}*/

		// update value
		pMacro->SetValue( pMacroValue );
	}
	else
	{
		// create a script type macro
		pMacro = new CMacro( pMacroName, pMacroValue, nullptr, false, bSetupDefineInProjectFile );
		_macros.InsertWithDupes( pMacroName, pMacro );
	}

	return pMacro;
}

//-----------------------------------------------------------------------------
// Property macros are a variant of script macros that are highly constrained and can only
// be used to capture the state of a property key within a configuration block. They can then
// only be resolved with a configuration block.
//-----------------------------------------------------------------------------
CMacro * CMacroStorage::SetAsProperty( const char *pMacroName, const char *pMacroValue, const char *pConfigurationName )
{
	g_pVPC->VPCStatus( false, "Set Property Macro (%s): $%s = %s", ( pConfigurationName && pConfigurationName[0] ? pConfigurationName : "???" ), pMacroName, pMacroValue );

	if ( !pConfigurationName || !pConfigurationName[0] )
	{
		// configuration is mandatory
		g_pVPC->VPCError( "Missing expected configuration for property macro '%s'.", pMacroName );
	}

	CMacro *pMacro = Get( pMacroName );
	if ( pMacro && !pMacro->IsPropertyMacro() )
	{
		// duplicate macro names are not allowed
		// found an existing non-property based macro with same name
		g_pVPC->VPCError( "Cannot set pre-existing macro '%s' as a property macro.", pMacroName );
	}

	// resolve with expected configuration
	pMacro = Get( pMacroName, pConfigurationName );
	if ( pMacro )
	{
		// update the macro
		pMacro->SetValue( pMacroValue );
	}
	else
	{
		// create property macro
		pMacro = new CMacro( pMacroName, pMacroValue, pConfigurationName, false, false );
		_macros.InsertWithDupes( pMacroName, pMacro );
	}

	return pMacro;
}

void CMacroStorage::GetPreprocessorDefines(char const* cfg_string, CUtlVector<CUtlString>& outDefines) const
{
	int nMacroCount = std::ranges::count_if(_macros, [this](MacroIdx idx)
	{
		return _macros[idx]->ShouldDefineInProjectFile();
	});


	// Add defines from $PreprocessorDefinitions
	CSplitString outStrings(cfg_string, g_IncludeSeparators, V_ARRAYSIZE(g_IncludeSeparators));

	outDefines.EnsureCapacity(outStrings.Count() + nMacroCount); // Presize to avoid realloc'ing and copying strings

	for (char const* cfg_define : outStrings)
		outDefines.AddToTail(cfg_define);


	// Add defines from VPC macros
	for (MacroIdx idx : _macros)
	{
		CMacro const* pMacro = _macros[idx];
		if (pMacro->ShouldDefineInProjectFile())
			outDefines.AddToTail(CFmtStrMax("%s=%s", pMacro->GetName(), pMacro->GetValue()).Get());
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CMacro * CMacroStorage::Get( const char *pMacroName, const char *pConfigurationName )
{
	if ( pConfigurationName && pConfigurationName[0] )
	{
		// iterate to find macro (duplicated due to configuration) with matching configuration
		for ( int nMacroIndex = _macros.FindFirst( pMacroName ); nMacroIndex != _macros.InvalidIndex(); nMacroIndex = _macros.NextInorderSameKey( nMacroIndex ) )
		{
			CMacro *pMacro = _macros[nMacroIndex];
			if ( pMacro->IsPropertyMacro() && !V_stricmp_fast( pConfigurationName, pMacro->GetConfigurationName() ) )
			{
				// found matching configuration based macro
				return pMacro;
			}
		}
	
		// not found
		return nullptr;
	}
	
	// direct lookup
	int nMacroIndex = _macros.Find( pMacroName );
	if ( nMacroIndex != _macros.InvalidIndex() )
	{
		return _macros[nMacroIndex];
	}

	// not found
	return nullptr;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CMacroStorage::GetMacrosMarkedForCompilerDefines( CUtlVector< CMacro* > &macroDefines )
{
	macroDefines.Purge();

	for ( int nMacroIndex = _macros.FirstInorder(); nMacroIndex != _macros.InvalidIndex(); nMacroIndex = _macros.NextInorder( nMacroIndex ) )
	{
		CMacro *pMacro = _macros[nMacroIndex];
		if ( pMacro->ShouldDefineInProjectFile() )
		{
			macroDefines.AddToTail( pMacro );
		}
	}

	return macroDefines.Count();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMacroStorage::ResolveString( char const *pString, CUtlStringBuilder *pOutBuff, CUtlVector< CUtlString > *pMacrosReplaced )
{
	// iterate and resolve user macros until all macros resolved
    if ( pString )
    {
        pOutBuff->Set( pString );
    }

    int nScanIndex = 0;
    while ( (size_t)nScanIndex < pOutBuff->Length() )
	{
        const char *pStartOfMacroToken = strchr( pOutBuff->Get() + nScanIndex, '$' );
        if ( !pStartOfMacroToken )
        {
            break;
        }

        // Skip over $.
        pStartOfMacroToken++;
        
        // If we don't find a macro for this $token we start scanning
        // right after the $. If we do find a macro and replace some
        // text we'll start right where we did the replacement at the $.
        nScanIndex = (int)( pStartOfMacroToken - pOutBuff->Get() );

        if ( !IsValidMacroNameChar( *pStartOfMacroToken ) )
        {
            continue;
        }
        
        char macroToken[MAX_MACRO_NAME];
        int nTokenChars = 0;

        CMacro *pMacro = nullptr;
		for ( int nMacroIndex = _macros.FirstInorder(); nMacroIndex != _macros.InvalidIndex(); nMacroIndex = _macros.NextInorder( nMacroIndex ) )
		{
			CMacro *pCheck = _macros[nMacroIndex];
            if ( ( nTokenChars <= 0 ||
                   pCheck->GetNameLength() >= nTokenChars ) &&
                 V_strnicmp( pStartOfMacroToken, pCheck->GetName(), pCheck->GetNameLength() ) == 0 )
            {
                //
                // resolve substring match as possible larger token for disqualifying an unintended replacement collision
                // i.e. $FOO cannot be replaced in a string that contains $FOOBAR, where $FOOBAR is a macro as well
                //

                // Collect token if we haven't already.
                if ( nTokenChars <= 0 )
                {
                    const char *pEndOfMacroToken = pStartOfMacroToken;
                    while ( *pEndOfMacroToken )
                    {
                        char ch = *pEndOfMacroToken;
                        if ( !IsValidMacroNameChar( ch ) )
                        {
                            break;
                        }
                        if ( nTokenChars < MAX_MACRO_NAME - 1 )
                        {
                            macroToken[nTokenChars] = ch;
                            nTokenChars++;
                        }
                        pEndOfMacroToken++;
                    }
                    macroToken[nTokenChars] = 0;

                    // We matched a macro name so there must
                    // be some legal token chars.
                    Assert( nTokenChars > 0 );
                }

                if ( pCheck->GetNameLength() < nTokenChars )
                {
                    if ( Get( macroToken ) )
                    {
                        // cannot replace this macro since it is colliding with the name of a larger macro.
                        // the iterations will converge to the correct macro.
                        continue;
                    }
                }

                pMacro = pCheck;
                break;
            }
        }
        if ( !pMacro )
        {
            continue;
        }

        if ( pMacro->HasConfigurationName() )
        {
            // property macros store a unique value for multiple configurations
			// TODO: Refactoring: uncouple this
            const char *configurationName = g_pVPC->GetProjectGenerator()->GetCurrentConfigurationName();
            if ( !configurationName || !configurationName[0] )
            {
                // no current configuration
                // trying to use a property macro outside a configuration block is nonsense
                // a property macro is paired to a configuration
				g_pVPC->VPCError( "Cannot use property macro '%s' in an expression outside of a configuration block", pMacro->GetName() );
            }

            if ( V_stricmp_fast( pMacro->GetConfigurationName(), configurationName ) )
            {
                // correct macro, but wrong configuration, get correct macro
                CMacro *pCorrectMacro = Get( pMacro->GetName(), configurationName );
                if ( !pCorrectMacro )
                {
                    // script expected macro to resolve
					g_pVPC->VPCError( "Property macro '%s' does not have an expected configuration '%s'.", pMacro->GetName(), configurationName );
                }
                else
                {
                    // this is the correct property macro with the expected configuration
                    pMacro = pCorrectMacro;
                }
            }
        }

        if ( pOutBuff->ReplaceAt( nScanIndex - 1, pMacro->GetFullNameLength(), pMacro->GetValue(), pMacro->GetValueLength() ) )
        {
            if ( pMacrosReplaced )
            {
                pMacrosReplaced->AddToTail( pMacro->GetFullName() );
            }
            
            // We replaced the text starting from the $ so restart
            // the scan there to pick up any new macro that came in.
            nScanIndex--;
        }
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMacroStorage::RemoveScriptCreated()
{
	// remove all the script created macros
	// this is to ensure the next project to be processed starts out with an unpolluted state
	for ( int nMacroIndex = _macros.FirstInorder(); nMacroIndex != _macros.InvalidIndex(); )
	{
		int nNextMacroIndex = _macros.NextInorder( nMacroIndex );

		CMacro *pMacro = _macros[nMacroIndex];
		if ( !pMacro->IsSystemMacro() )
		{			
			_macros.RemoveAt( nMacroIndex );
			delete pMacro;
		}

		nMacroIndex = nNextMacroIndex;
	}
}

const char * CMacroStorage::GetValue( const char *pMacroName, const char *pConfigurationName )
{
	CMacro *pMacro = Get( pMacroName, pConfigurationName );
	if ( !pMacro ) return ""; // not found
		
		
	if ( pMacro->IsPropertyMacro() && ( !pConfigurationName || !pConfigurationName[0] ) )
	{
		g_pVPC->VPCError( "Missing required configuration to access property macro '%s'.", pMacroName );
	}

	return pMacro->GetValue();
}
