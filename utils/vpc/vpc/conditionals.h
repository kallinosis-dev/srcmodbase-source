#pragma once
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"


enum conditionalType_e
{
	CONDITIONAL_NULL,
	CONDITIONAL_PLATFORM,	// reserved for each known platform
	CONDITIONAL_GAME,		// reserved for each known game
	CONDITIONAL_SYSTEM,		// reserved for system features that permute global state that cannot be altered per script, not changeable by user scripts.
	CONDITIONAL_CUSTOM,		// created by user via command line, used for private or local testing
	CONDITIONAL_SCRIPT,		// created by scripts
};


struct conditional_t
{
	conditional_t()
	{
		m_Type = CONDITIONAL_NULL;
		m_bDefined = false;
		m_bGameConditionActive = false;
	}

	conditional_t(const conditional_t& other) = default;
	

	CUtlString			m_Name;
	CUtlString			m_UpperCaseName;
	conditionalType_e	m_Type;

	// a conditional can be present in the table but not defined
	// e.g. default conditionals that get set by command line args
	bool				m_bDefined;

	// only used during multiple game iterations for game conditionals as each 'defined' game becomes active
	bool				m_bGameConditionActive;
};

class CConditionalStorage
{
	CUtlVector< conditional_t* >	_conditionals;

public:
	// Returns the mask identifying what platforms should be built
	bool					IsPlatformDefined(const char* pName);
	bool					IsPlatformName(const char* pName);
	const char* GetTargetPlatformName();
	const char* GetTargetCompilerName();

	conditional_t* FindOrCreateConditional(const char* pName, bool bCreate, conditionalType_e type);
	bool					ResolveConditionalSymbol(const char* pSymbol);
	bool					EvaluateConditionalExpression(const char* pExpression);
	bool					ConditionHasDefinedType(const char* pCondition, conditionalType_e type);
	void					SetConditional(const char* pName, bool bSet, conditionalType_e type);
	bool					IsConditionalDefined(const char* pName);

	void					SetupDefaultConditionals();
	CUtlString				GetCRCStringFromConditionals();

};