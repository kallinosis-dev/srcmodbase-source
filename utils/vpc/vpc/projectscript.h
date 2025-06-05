#pragma once
#include "tier1/checksum_crc.h"
#include "tier1/utlstring.h"

typedef int scriptIndex_t;
struct script_t
{
	CUtlString		name;
	CUtlString		m_condition;
};



struct scriptList_t
{
	scriptList_t()
	{
		m_crc = 0;
	}

	CUtlString	m_scriptName;
	CRC32_t		m_crc;
	bool		m_bCRCCheck;
};

extern void VPC_ParseProjectScriptParameters(const char* szScriptName, int depth, bool bQuiet);
extern void VPC_HandleProjectCommands(const char* pUnusedScriptName, int depth, bool bQuiet);