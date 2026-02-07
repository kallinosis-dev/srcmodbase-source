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