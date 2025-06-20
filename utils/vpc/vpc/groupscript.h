#pragma once
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

struct script_t;


using projectIndex_t = int;
struct project_t
{
	CUtlString				name;
	CUtlVector< script_t >	scripts;
};

using groupIndex_t = int;
struct group_t
{
	CUtlVector< projectIndex_t >	projects;
};

using groupTagIndex_t = int;
struct groupTag_t
{
	groupTag_t()
	{
		bSameAsProject = false;
	}

	CUtlString					name;
	CUtlVector< groupIndex_t >	groups;

	// this tag is an implicit definition of the project
	bool						bSameAsProject;
};


extern void					VPC_ParseGroupScript(const char* pScriptName);
extern groupTagIndex_t VPC_Group_FindOrCreateGroupTag(const char* pName, bool bCreate);
extern projectIndex_t VPC_Group_FindOrCreateProject(const char* pName, bool bCreate);