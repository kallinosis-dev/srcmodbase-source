#include "scriptutil.h"

#include "logging.h"
#include "strtools.h"

MaybeBool Script_ParseMaybeBool(char const* str)
{
	if ( !V_strnicmp( str, "no", 2 ) || 
		!V_strnicmp_fast( str, "off", 3 ) || 
		!V_strnicmp_fast( str, "false", 5 ) || 
		!V_strnicmp_fast( str, "not set", 7 ) || 
		!V_strnicmp_fast( str, "disabled", 8 ) || 
		!V_strnicmp_fast( str, "0", 1 ) )
	{
		// false
		return MaybeBool::False;
	}

	if ( !V_strnicmp_fast( str, "yes", 3 ) || 
		!V_strnicmp_fast( str, "on", 2 ) || 
		!V_strnicmp_fast( str, "true", 4  ) || 
		!V_strnicmp_fast( str, "set", 3 ) || 
		!V_strnicmp_fast( str, "enabled", 7 ) || 
		!V_strnicmp_fast( str, "1", 1 ) )
	{
		// true
		return MaybeBool::True;
	}

	return MaybeBool::Unknown;
}

bool Script_ParseBool(char const* str, const CScript* src)
{
	MaybeBool val = Script_ParseMaybeBool(str);

	if(val == MaybeBool::Unknown)
	{
		logging::SyntaxError( src, "Unknown boolean expression '%s'", str);
	}

	return val == MaybeBool::True;
}


//--------------------------------------------------------------------------------
// Does the line containing the given cursor start with a '//' comment?
//--------------------------------------------------------------------------------
bool Script_IsSingleLineComment( const char *pSearchPos, const char *pFileStart )
{
	// Rewind to line start
	while( pSearchPos > pFileStart )
	{
		if ( ( pSearchPos[-1] == '\n' ) || ( pSearchPos[-1] == '\r' ) )
			break;
		pSearchPos--;
	}
	// Skip past whitespace
	while( V_isspace( pSearchPos[0] ) ) pSearchPos++;
	// Return true if the first non-whitespace characters on the line are '//'
	return ( pSearchPos[0] == '/' ) && ( pSearchPos[1] == '/' );
}



const char *Script_EvaluateEnvironmentExpression( const char *pExpression, const char *pDefault )
{
	bool bEnvDefinedMacro = false;
	char *pEnvVarName = (char*)StringAfterPrefix( pExpression, "$env(" );
	if ( !pEnvVarName )
	{
		// not an environment specification
		pEnvVarName = (char*)StringAfterPrefix( pExpression, "$envdefined(" );
		if ( !pEnvVarName )
		{
			return nullptr;
		}
		bEnvDefinedMacro = true;
	}
	
	char *pLastChar = &pEnvVarName[ V_strlen( pEnvVarName ) - 1 ];
	if ( !*pEnvVarName || *pLastChar != ')' )
	{
		logging::SyntaxError( TODO, "%s must have a closing ')' in \"%s\"\n", bEnvDefinedMacro ? "$envdefined()" : "$env()", pExpression);
	}

	// get the contents of the $env( blah..blah ) expressions
	// handles expresions that could have whitepsaces
	g_pVPC->GetScript().PushScript( pExpression, pEnvVarName, 1, false, false );
	const char *pToken = g_pVPC->GetScript().GetToken( false );
	g_pVPC->GetScript().PopScript();

	if ( pToken && pToken[0] )
	{
		const char *pResolve = getenv( pToken );
        if ( bEnvDefinedMacro )
		{
			return pResolve ? "1" : "0";
		}
		else
		{
			if ( pResolve )
			{
				return pResolve;
			}
		}
    }

    return pDefault;
}