//========= Copyright � 1996-2016, Valve Corporation, All rights reserved. ============//
//
// Purpose: VPC
//
//=====================================================================================//

#include "ibaseprojectgenerator.h"
#include "macros.h"
#include "misc.h"
#include "projectscript.h"
#include "scriptutil.h"
#include "vpc.h"

static KeywordName_t s_KeywordNameTable[] =
{
	{"$General",				KEYWORD_GENERAL},
	{"$Debugging",				KEYWORD_DEBUGGING},
	{"$Compiler",				KEYWORD_COMPILER},
	{"$Librarian",				KEYWORD_LIBRARIAN},
	{"$Linker",					KEYWORD_LINKER},
	{"$ManifestTool",			KEYWORD_MANIFEST},
	{"$XMLDocumentGenerator",	KEYWORD_XMLDOCGEN},
	{"$BrowseInformation",		KEYWORD_BROWSEINFO},
	{"$Resources",				KEYWORD_RESOURCES},
	{"$PreBuildEvent",			KEYWORD_PREBUILDEVENT},
	{"$PreLinkEvent",			KEYWORD_PRELINKEVENT},
	{"$PostBuildEvent",			KEYWORD_POSTBUILDEVENT},
	{"$CustomBuildStep",		KEYWORD_CUSTOMBUILDSTEP},
	{"$Ant",					KEYWORD_ANT},
	{"$Intellisense",			KEYWORD_INTELLISENSE},
};

const char *CVPC::KeywordToName( configKeyword_e keyword )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE( s_KeywordNameTable ) == KEYWORD_MAX );

	if ( keyword == KEYWORD_UNKNOWN )
	{
		return "???";
	}

	return s_KeywordNameTable[keyword].m_pName;
}

configKeyword_e CVPC::NameToKeyword( const char *pKeywordName )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE( s_KeywordNameTable ) == KEYWORD_MAX );

	for ( int i = 0; i < ARRAYSIZE( s_KeywordNameTable ); i++ )
	{
		if ( !V_stricmp_fast( pKeywordName, s_KeywordNameTable[i].m_pName ) )
		{
			return s_KeywordNameTable[i].m_Keyword;
		}
	}

	return KEYWORD_UNKNOWN;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProjectScriptParser::Config_Macro()
{
	// Allowing macros to be created/set inside of configurations in order to construct a macro that is vectored on the configuration
	const char *pToken = _script->GetToken( false );
	if ( !pToken || !pToken[0] )
	{
		logging::SyntaxError(_script);
	}

    CUtlStringHolder<MAX_MACRO_NAME> macroName( pToken );

    CUtlStringBuilder *pStrBuf = g_pVPC->GetPropertyValueBuffer();
	if ( !_script->ParsePropertyValue(nullptr, pStrBuf ) )
	{
		return;
	}

	g_pVPC->macros.SetAsProperty( macroName, pStrBuf->Get(), _projgen->GetCurrentConfigurationName() );
}

//-----------------------------------------------------------------------------
//	VPC_Config_Keyword
//
//-----------------------------------------------------------------------------
void CProjectScriptParser::Config_Keyword( configKeyword_e keyword, const char *pkeywordToken )
{
	const char		*pToken;

	if ( ( keyword == KEYWORD_LIBRARIAN ) && g_pVPC->IsProjectUsingUnity() )
	{
		logging::Warning( "$UnityProject should not be used in .LIB projects! The unity build generates a few large OBJs, so due to per-OBJ linker dependency determination linking with a unity-built .LIB would thus incur many unnecessary link dependencies." );
	}

	bool bShouldSkip = false;
	if ( !_projgen->StartPropertySection( keyword, &bShouldSkip ) )
	{
		logging::SyntaxError( _script, "Unsupported Keyword: %s for target platform", pkeywordToken);
	}

	if ( bShouldSkip )
	{
		pToken = _script->PeekNextToken( true );
		if ( !pToken || !pToken[0] || !CharStrEq( pToken, '{' ) )
			logging::SyntaxError(_script);

		_script->SkipBracedSection();
	}
	else
	{
		pToken = _script->GetToken( true );
		if ( !pToken || !pToken[0] || !CharStrEq( pToken, '{' ) )
			logging::SyntaxError(_script);
		
		while ( 1 )
		{
			pToken = _script->GetToken( true );
			if ( !pToken || !pToken[0] )
				break;

			if ( CharStrEq( pToken, '}' ) )
			{
				// end of section
				break;
			}

			// Copy off the token name so HandleProperty() doesn't have to (or else the parser will overwrite it on the next token).
            CUtlStringHolder<100> tempTokenName( pToken );

			if ( !V_stricmp_fast( tempTokenName, "$PropertyMacro" ) )
			{
				// Allowing macros to be created/set inside of configurations in order to save off a property state into a macro.
				// This provides a way for users to temp alter properties and then restore them.
				// Syntax: $Macro <MacroName> <PropertyName> [condition]
				pToken = _script->GetToken( false );
				if ( !pToken || !pToken[0] )
					logging::SyntaxError(_script);

                CUtlStringHolder<MAX_MACRO_NAME> macroName( pToken );

				// resolve the token that should be a recognized property key
                CUtlStringBuilder *pStrBuf = g_pVPC->GetPropertyValueBuffer();
				if ( !_script->ParsePropertyValue(nullptr, pStrBuf ) )
				{
					continue;
				}

				// get the specified property key's value and set it
				g_pVPC->macros.SetAsProperty( macroName, _projgen->GetPropertyValue( pStrBuf->Get() ), _projgen->GetCurrentConfigurationName() );
			}
			else if ( !V_stricmp_fast( tempTokenName, "$Macro" ) )
			{
				Config_Macro();
			}
			else
			{
				_projgen->HandleProperty( tempTokenName );
			}
		}
	}

	_projgen->EndPropertySection( keyword );
}

//-----------------------------------------------------------------------------
//	VPC_Keyword_Configuration
//
//-----------------------------------------------------------------------------
void CProjectScriptParser::Keyword_Configuration()
{
	//determine project generator before any generator-dependent configuration is allowed
	g_pVPC->DetermineProjectGenerator();

	const char				*pToken;
	CUtlStringHolder<50>	configName;
	bool					bAllowNextLine = false;
	CUtlVector<CUtlString>	configs;

	while ( 1 )
	{
		pToken = _script->GetToken( bAllowNextLine );
		if ( !pToken || !pToken[0] )
			break;

		if ( CharStrEq( pToken, '\\' ) )
		{
			bAllowNextLine = true;
			continue;
		}
		else
		{
			bAllowNextLine = false;
		}

		int index = configs.AddToTail();
		configs[index] = pToken;

		// check for another optional config
		pToken = _script->PeekNextToken( bAllowNextLine );
		if ( !pToken || !pToken[0] || CharStrEq( pToken, '{' ) || CharStrEq( pToken, '}' ) || (pToken[0] == '$') )
			break;
	}

	// no configuration specified, use all known
	if ( !configs.Count() )
	{
		_projgen->GetAllConfigurationNames( configs );
		if ( !configs.Count() )
		{
			logging::Error( "Trying to parse a configuration block and no configs have been defined yet.\n[%s line:%d]", _script->GetName(), _script->GetLine() );
		}
	}

	// save parser state
	CScriptSource scriptSource = _script->GetCurrentScript();

	for ( int i = 0; i < configs.Count(); i++ )
	{
		// restore parser state
		_script->RestoreScript( scriptSource );

        configName.Set( configs[i].String() );

		// get access objects
		_projgen->StartConfigurationBlock( configName, false );

		pToken = _script->GetToken( true );
		if ( !pToken || !pToken[0] || !CharStrEq( pToken, '{' ) )
		{
			logging::SyntaxError(_script);
		}

		while ( 1 )
		{
			_script->SkipToValidToken();

			pToken = _script->PeekNextToken( true );
			if ( pToken && pToken[0] && !V_stricmp_fast( pToken, "$Macro" ) )
			{
				pToken = _script->GetToken( true );
				if ( !pToken  || !pToken[0] )
					logging::SyntaxError(_script);

				Config_Macro();
				continue;
			}

            CUtlStringBuilder *pStrBuf = g_pVPC->GetPropertyValueBuffer();
			if ( !_script->ParsePropertyValue(nullptr, pStrBuf ) )
			{
				_script->SkipBracedSection();
				continue;
			}

			if ( CharStrEq( pStrBuf->Get(), '}' ) )
			{
				// end of section
				break;
			}

			configKeyword_e keyword = g_pVPC->NameToKeyword( pStrBuf->Get() );
			if ( keyword == KEYWORD_UNKNOWN )
			{
				logging::SyntaxError(_script);
			}
			else
			{
                CUtlStringHolder<50> keywordStr( pStrBuf->Get() );
				Config_Keyword( keyword, keywordStr );
			}
		}

		_projgen->EndConfigurationBlock();
	}
}

//-----------------------------------------------------------------------------
//	VPC_Keyword_FileConfiguration
//
//-----------------------------------------------------------------------------
void CProjectScriptParser::Keyword_FileConfiguration()
{
	const char	*pToken;
	bool		bAllowNextLine = false;
	CUtlVector< CUtlString > configurationNames;

	while ( 1 )
	{
		pToken = _script->GetToken( bAllowNextLine );
		if ( !pToken || !pToken[0] )
			break;

		if ( CharStrEq( pToken, '\\' ) )
		{
			bAllowNextLine = true;
			continue;
		}
		else
		{
			bAllowNextLine = false;
		}

		configurationNames.AddToTail( pToken );

		// check for another optional config
		pToken = _script->PeekNextToken( bAllowNextLine );
		if ( !pToken || !pToken[0] || CharStrEq( pToken, '{' ) || CharStrEq( pToken, '}' ) || (pToken[0] == '$') )
			break;
	}

	// no configuration specified, use all known
	if ( configurationNames.Count() == 0 )
	{
		_projgen->GetAllConfigurationNames( configurationNames );
	}

	// save parser state
	CScriptSource scriptSource = _script->GetCurrentScript();

    int nWarningLine = -1;

	for ( int i=0; i < configurationNames.Count(); i++ )
	{
		// restore parser state
		_script->RestoreScript( scriptSource );

		// Tell the generator we're about to feed it configuration data for this file.
		_projgen->StartConfigurationBlock( configurationNames[i].String(), true );

		pToken = _script->GetToken( true );
		if ( !pToken || !pToken[0] || !CharStrEq( pToken, '{' ) )
		{
			logging::SyntaxError(_script);
		}

		while ( 1 )
		{
			_script->SkipToValidToken();

			pToken = _script->PeekNextToken( true );
			if ( pToken && pToken[0] && !V_stricmp_fast( pToken, g_pOption_ExcludedFromBuild ) )
			{
				pToken = _script->GetToken( true );
				if ( !pToken || !pToken[0] )
					logging::SyntaxError(_script);

                CUtlStringBuilder *pStrBuf = g_pVPC->GetPropertyValueBuffer();
				if ( _script->ParsePropertyValue(nullptr, pStrBuf ) )
				{
					_projgen->FileExcludedFromBuild( Script_ParseBool( pStrBuf->Get(), _script ) );
				}

				continue;
			}
			else if ( pToken && pToken[0] && !V_stricmp_fast( pToken, "$Macro" ) )
			{
				pToken = _script->GetToken( true );
				if ( !pToken || !pToken[0] )
					logging::SyntaxError(_script);

				Config_Macro();
				continue;
			}

            CUtlStringBuilder *pStrBuf = g_pVPC->GetPropertyValueBuffer();
			if ( !_script->ParsePropertyValue(nullptr, pStrBuf ) )
			{
				_script->SkipBracedSection();
				continue;
			}

			if ( CharStrEq( pStrBuf->Get(), '}' ) )
			{
				// end of section
				break;
			}

			configKeyword_e keyword = g_pVPC->NameToKeyword( pStrBuf->Get() );
			switch ( keyword )
			{
			// these are the only tools wired to deal with file configuration overrides
			case KEYWORD_COMPILER:
                if ( !g_pVPC->IsPerFileCompileConfigEnabled() &&
                     !_script->IsInPrivilegedScript() &&
                     _script->GetLine() != nWarningLine )
                {
                    logging::SyntaxError( _script, "%s(%u): per-file compile configuration not allowed", _script->GetName(), _script->GetLine());
                    nWarningLine = _script->GetLine();
                }
                // Fall through
			case KEYWORD_RESOURCES:
			case KEYWORD_CUSTOMBUILDSTEP:
            {
                CUtlStringHolder<50> keywordStr( pStrBuf->Get() );
				Config_Keyword( keyword, keywordStr );
				break;
            }
			default:
				logging::SyntaxError(_script);
			}
		}
		
		_projgen->EndConfigurationBlock();
	}
}

