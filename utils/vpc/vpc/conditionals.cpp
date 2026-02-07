//======== Copyright � 1996-2016, Valve Corporation, All rights reserved. ===========//
//
// Purpose: VPC
//
//=====================================================================================//

#include "conditionals.h"

#include <ranges>
#include "tier1/fmtstr.h"
#include "misc.h"
#include "vpc.h"

struct CMacro;
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CConditionalStorage::SetupDefaultConditionals()
{
	//
	// PLATFORM Conditionals
	//
	{
		CreateOrGet( "WIN32", CONDITIONAL_PLATFORM );
		CreateOrGet( "WIN64", CONDITIONAL_PLATFORM );
		
		CreateOrGet( "LINUX32", CONDITIONAL_PLATFORM );
		CreateOrGet( "LINUX64", CONDITIONAL_PLATFORM );
		CreateOrGet( "LINUXSERVER32", CONDITIONAL_PLATFORM );
		CreateOrGet( "LINUXSERVER64", CONDITIONAL_PLATFORM );
		CreateOrGet( "LINUXSTEAMRTARM32HF", CONDITIONAL_PLATFORM );
		CreateOrGet( "LINUXSTEAMRTARM64HF", CONDITIONAL_PLATFORM );

		CreateOrGet( "OSX32", CONDITIONAL_PLATFORM );
		CreateOrGet( "OSX64", CONDITIONAL_PLATFORM );

		CreateOrGet( "IOS", CONDITIONAL_PLATFORM );

		CreateOrGet( "ANDROIDARM32", CONDITIONAL_PLATFORM );
		CreateOrGet( "ANDROIDARM64", CONDITIONAL_PLATFORM );
		CreateOrGet( "ANDROIDMIPS32", CONDITIONAL_PLATFORM );
		CreateOrGet( "ANDROIDMIPS64", CONDITIONAL_PLATFORM );
		CreateOrGet( "ANDROIDX8632", CONDITIONAL_PLATFORM );
		CreateOrGet( "ANDROIDX8664", CONDITIONAL_PLATFORM );
	}
	
	//
	// SYSTEM conditionals
	//
	{
		// setup default system conditionals
		CreateOrGet( "PROFILE", CONDITIONAL_SYSTEM );
		CreateOrGet( "RETAIL", CONDITIONAL_SYSTEM );
		CreateOrGet( "CALLCAP", CONDITIONAL_SYSTEM );
		CreateOrGet( "FASTCAP", CONDITIONAL_SYSTEM );
		CreateOrGet( "MEMTEST", CONDITIONAL_SYSTEM );
		CreateOrGet( "NOFPO", CONDITIONAL_SYSTEM );
		CreateOrGet( "POSIX", CONDITIONAL_SYSTEM );
		CreateOrGet( "LV", CONDITIONAL_SYSTEM );
		CreateOrGet( "DEMO", CONDITIONAL_SYSTEM );
		CreateOrGet( "DVDEMU", CONDITIONAL_SYSTEM );
		CreateOrGet( "QTDEBUG", CONDITIONAL_SYSTEM );
		CreateOrGet( "NO_CEG", CONDITIONAL_SYSTEM );
		CreateOrGet( "UPLOAD_CEG", CONDITIONAL_SYSTEM );
		CreateOrGet( "SOURCECONTROL", CONDITIONAL_SYSTEM );
		CreateOrGet( "ALLOW_OS_MACRO", CONDITIONAL_SYSTEM );
		CreateOrGet( "CRCCHECK_IN_PROJECT", CONDITIONAL_SYSTEM );
		CreateOrGet( "MISSING_FILE_CHECK", CONDITIONAL_SYSTEM );
		CreateOrGet( "MISSING_FILE_IS_ERROR", CONDITIONAL_SYSTEM );
		CreateOrGet( "FILEPATTERN", CONDITIONAL_SYSTEM );
		CreateOrGet( "ADD_EXE_TO_CRC_CHECK", CONDITIONAL_SYSTEM );
		CreateOrGet( "ALLOW_QT", CONDITIONAL_SYSTEM );
		CreateOrGet( "ALLOW_SCHEMA", CONDITIONAL_SYSTEM );
		CreateOrGet( "ALLOW_UNITY", CONDITIONAL_SYSTEM );
		CreateOrGet( "ALLOW_CLANG", CONDITIONAL_SYSTEM );
	}
}	

CUtlString CConditionalStorage::GetCRCStringFromConditionals()
{
	CUtlString CRCString;

	CUtlVectorFixedGrowable<const char *, 1024> sortRelevantConditionals;

	// Any enabled system conditional needs to make a CRC string that can be matched against for project staleness.
	// These used to be terse abbreviations when they were passed on the CL but now not a constraint with vpccrccheck and peer crc files.
	for ( conditional_t const* cond: _conditionals)
	{
		if (cond->m_bDefined &&
			(cond->m_Type == CONDITIONAL_SYSTEM || cond->m_Type == CONDITIONAL_CUSTOM || cond->m_Type == CONDITIONAL_SCRIPT))
		{
			sortRelevantConditionals.AddToTail(cond->m_UpperCaseName.Get());
		}
	}

	//sort the conditionals so the existing crc doesn't appear stale if they simply appear in a different order on the command line
	sortRelevantConditionals.SortPredicate( 
		[] ( const char *szLeft, const char *szRight ) -> bool
		{
			return V_stricmp_fast( szLeft, szRight ) < 0;
		} );

	for ( int i = 0; i < sortRelevantConditionals.Count(); ++i )
	{
		CRCString += CFmtStr( ".%s", sortRelevantConditionals[i] );
	}

	return CRCString;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char * CConditionalStorage::GetTargetPlatformName()
{
	auto platform_cond = std::ranges::find_if(
		_conditionals, [](conditional_t const* cond)
		{
			return cond->m_Type == CONDITIONAL_PLATFORM && cond->m_bDefined;
		});


	if(platform_cond == _conditionals.end())
	{
		// fatal - should have already been default set
		Assert(0);
		logging::Error("Unspecified platform.");

		return nullptr;
	}

	return (*platform_cond)->m_Name.Get();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char * CConditionalStorage::GetTargetCompilerName()
{
	const char *pPlatformName = GetTargetPlatformName();
	if ( !V_stricmp_fast( pPlatformName, "WIN32" ) ||
		 !V_stricmp_fast( pPlatformName, "WIN64" ))
	{
		if ( IsDefined( "VS2005" ) )
			return "VS2005";

		if ( IsDefined( "VS2010" ) )
			return "VS2010";

		if ( IsDefined( "VS2012" ) )
			return "VS2012";

		if ( IsDefined( "VS2013" ) )
			return "VS2013";

		if ( IsDefined( "VS2015" ) )
			return "VS2015";

		if ( IsDefined( "VS2022" ) )
			return "VS2022";
	}
    else if ( VPC_IsPlatformLinux( pPlatformName ) || VPC_IsPlatformAndroid( pPlatformName ) )
    {
        return "GCC";
    }
	else if ( !V_stricmp_fast( pPlatformName, "OSX32" ) ||
              !V_stricmp_fast( pPlatformName, "OSX64" ) )
	{
        return "Clang";
    }

	// TODO: support other platforms (needed by schemacompiler/clang)
	ExecuteOnce( logging::Warning( "TODO: GetTargetCompilerName not yet implemented for platform %s!", pPlatformName ) );
	return "UNKNOWN";
}
//-----------------------------------------------------------------------------
//	Case Insensitive. Returns true if platform conditional has been marked
//	as defined.
//-----------------------------------------------------------------------------
bool CConditionalStorage::IsPlatformDefined( const char *pName )
{
	for ( int i = 0; i < _conditionals.Count(); i++ )
	{
		if ( _conditionals[i]->m_Type == CONDITIONAL_PLATFORM && !V_stricmp_fast( pName, _conditionals[i]->m_Name.String() ) )
		{
			return _conditionals[i]->m_bDefined;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//	Case Insensitive. Returns true if the given string is a platform name
//-----------------------------------------------------------------------------
bool CConditionalStorage::IsPlatformName( const char *pName )
{
	for ( int i=0; i<_conditionals.Count(); i++ )
	{
		if ( _conditionals[i]->m_Type == CONDITIONAL_PLATFORM && !V_stricmp_fast( pName, _conditionals[i]->m_Name.String() ) )
		{
			return true;
		}
	}
	return false;
}



conditional_t* CConditionalStorage::Get(char const* pName)
{
	for (conditional_t* cond : _conditionals)
	{
		if (!V_stricmp_fast(pName, cond->m_Name.String()))
			return cond;
	}

	return nullptr;
}


//-----------------------------------------------------------------------------
//	Case Insensitive
//-----------------------------------------------------------------------------
conditional_t * CConditionalStorage::CreateOrGet( const char *pName, conditionalType_e type )
{

	if(auto cond = Get(pName))
	{
		return cond;
	}

	int index = _conditionals.AddToTail();
	_conditionals[index] = new conditional_t();

	char tempName[256];
	V_strncpy( tempName, pName, sizeof( tempName ) );
	
	// primary internal use as lower case, but spewed to user as upper for style consistency
	_conditionals[index]->m_Name = V_strlower( tempName );
	_conditionals[index]->m_UpperCaseName = V_strupper( tempName );
	_conditionals[index]->m_Type = type;

	return _conditionals[index];
}

void CConditionalStorage::Set( const char *pString, bool bSet, conditionalType_e conditionalType )
{
	conditional_t *pConditional = CreateOrGet( pString, conditionalType );
	if ( !pConditional )
	{
		logging::Error( "Failed to find or create $%s conditional", pString );
	}

	logging::Status( false, "Set Conditional: $%s = %s", pConditional->m_UpperCaseName.Get(), ( bSet ? "1" : "0" ) );

	if ( conditionalType != pConditional->m_Type )
	{
		logging::SyntaxError( TODO, "Cannot set reserved conditional '$%s'", pConditional->m_UpperCaseName.Get());
	}

	pConditional->m_bDefined = bSet;

	if ( pConditional->m_Type == CONDITIONAL_SYSTEM )
	{
		g_pVPC->SetSystemConditional(pConditional->m_UpperCaseName.Get(), bSet);

	}
}

//-----------------------------------------------------------------------------
//	Returns true if string has a conditional of the specified type
//-----------------------------------------------------------------------------
bool CConditionalStorage::ConditionHasDefinedType( const char* pCondition, conditionalType_e type )
{
	for ( int i=0; i<_conditionals.Count(); i++ )
	{
		if ( _conditionals[i]->m_Type != type )
			continue;

        const char *pScan = pCondition;
        while ( *pScan )
        {
            pScan = strchr( pScan, '$' );
            if ( !pScan )
            {
                break;
            }

            pScan++;
            if ( V_strnicmp( pScan, _conditionals[i]->m_Name, _conditionals[i]->m_Name.Length() ) == 0 )
            {
                // a define of expected type occurs in the conditional expression
                return true;
            }
        }
	}

	return false;
}

//-----------------------------------------------------------------------------
//	Callback for expression evaluator.
//-----------------------------------------------------------------------------
bool CConditionalStorage::ResolveConditionalSymbol( const char *pSymbol )
{
	int offset = 0;

	if ( ( pSymbol[0] == '$' && pSymbol[1] == '0' && pSymbol[2] == 0 ) ||
         CharStrEq( pSymbol, '0' ) )
	{
		return false;
	}
	else if ( ( pSymbol[0] == '$' && pSymbol[1] == '1' && pSymbol[2] == 0 ) ||
              CharStrEq( pSymbol, '1' ) )
	{
		return true;
	}

	if ( pSymbol[0] == '$' )
	{
		offset = 1;
	}

	if ( conditional_t *pConditional = Get( pSymbol+offset ) )
	{
		// game conditionals only resolve true when they are 'defined' and 'active'
		// only one game conditional is expected to be active at a time
		if ( pConditional->m_Type == CONDITIONAL_GAME )
		{
			if ( !pConditional->m_bDefined )
			{
				return false;
			}

			return pConditional->m_bGameConditionActive;
		}

		// all other type of conditions are gated by their 'defined' state
		return pConditional->m_bDefined;
	}
	else
	{
		// The conditional was not found.
		// MACROS ARE NOT ALLOWED IN CONDITIONALS because it became too commonplace to use the wrong symbol
		// causing quiet unintended results and since macros can be arbitrary strings, there placement in a
		// conditional expression is invalid. Restricting to conditionals ensures we are only ever evaluating
		// is valid boolean values.
		CMacro *pMacro = g_pVPC->macros.Get( (char*)pSymbol+offset );
		if ( pMacro )
		{
			// found a macro, and not allowed
			logging::SyntaxError( TODO, "Macro '%s' detected in conditional expression and not allowed. Use \"$Conditional <name> <0/1>\"", pSymbol);
		}
	}

	// unknown conditional, defaults to false
	return false;
}

//-----------------------------------------------------------------------------
//	Callback for expression evaluator.
//-----------------------------------------------------------------------------
static bool ResolveSymbol( const char *pSymbol )
{
	return g_pVPC->conditionals.ResolveConditionalSymbol( pSymbol );
}

//-----------------------------------------------------------------------------
//	Callback for expression evaluator.
//-----------------------------------------------------------------------------
static void SymbolSyntaxError( const char *pReason )
{
	// invoke internal syntax error hndling which spews script stack as well
	logging::SyntaxError( TODO, "%s", pReason);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CConditionalStorage::EvaluateConditionalExpression( const char *pExpression )
{
	if ( !pExpression || !pExpression[0] )
	{
		// empty string, same as not having a conditional
		return true;
	}

	bool bResult = false;
	CExpressionEvaluator ExpressionHandler;
	bool bValid = ExpressionHandler.Evaluate( bResult, pExpression, ::ResolveSymbol, ::SymbolSyntaxError );
	if ( !bValid )
	{
		logging::SyntaxError( TODO, "VPC Conditional Evaluation Error");
	}

	return bResult;
}

bool CConditionalStorage::IsDefined( const char *pName )
{
	conditional_t *pConditional = Get( pName );
	return pConditional && pConditional->m_bDefined;
}



CConditionalStorage::GetAllRange CConditionalStorage::GetAll(conditionalType_e type)
{
	ConditionalTypePredicate pred{ type };

	return _conditionals | std::ranges::views::filter(pred);
}

CConditionalStorage::GetAllDefinedRange CConditionalStorage::GetAllDefined(conditionalType_e type)
{
	DefinedConditionalTypePredicate pred{ type };

	return _conditionals | std::ranges::views::filter(pred);
}

CConditionalStorage::Storage const& CConditionalStorage::GetStorage() const
{
	return _conditionals;
}

bool CConditionalStorage::HasAny() const
{
	return _conditionals.Count() != 0;
}


// ---------
// TODO: cut this

void CVPC::SaveConditionals()
{
	// only expecting a single save point
	AssertMsg(m_SavedConditionals.Count() == 0, "SaveConditionals: Unexpected processing state, conditionals already saved\n");

	m_SavedConditionals.Purge();

	if (!conditionals.HasAny())
		return;

	// clone
	CConditionalStorage::Storage const& curStorage = conditionals.GetStorage();

	m_SavedConditionals.SetCount(curStorage.Count());
	for (int i = 0; i < curStorage.Count(); i++)
	{
		m_SavedConditionals[i] = new conditional_t(*curStorage[i]);
	}
}

void CVPC::RestoreConditionals()
{
	if (!m_SavedConditionals.Count())
		// already restored or nothing saved
		return;

	// This function is already a hack, extra const_cast wouldn't make it worse.
	auto curStorage = const_cast<CConditionalStorage::Storage *>(&conditionals.GetStorage());

	// whatever state the conditionals were changed to is undesired
	// these get discarded
	curStorage->PurgeAndDeleteElements();

	// restore the saved conditionals and purge the saved
	curStorage->Swap(m_SavedConditionals);
	for (conditional_t* cond : *curStorage)
	{
		// Call SetConditional to update cached member bools:
		conditionals.Set(cond->m_Name.Get(), cond->m_bDefined, cond->m_Type);
	}
}
