#pragma once

class CScript;

enum class MaybeBool: char8_t
{
	False = 0,
	True = 1,
	Unknown
};


//-----------------------------------------------------------------------------
//	Ignores allowable trailing characters.
//-----------------------------------------------------------------------------
MaybeBool Script_ParseMaybeBool(char const* str);
bool Script_ParseBool(char const* str, const CScript* src);

bool	Script_IsSingleLineComment( const char *pSearchPos, const char *pFileStart );