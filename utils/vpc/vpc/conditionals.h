#pragma once
#include <ranges>

#include "conditionals.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"


class CScript;

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

public:
	using Storage = CUtlVector< conditional_t* >;

	// Returns the mask identifying what platforms should be built
	bool					IsPlatformDefined(const char* pName);
	bool					IsPlatformName(const char* pName);
	const char* GetTargetPlatformName();
	const char* GetTargetCompilerName();

	conditional_t*			Get(char const* pName);
	conditional_t*			CreateOrGet(const char* pName, conditionalType_e type);
	bool					ResolveConditionalSymbol(const char* pSymbol, /*nullable*/ CScript const* script);
	bool					EvaluateConditionalExpression(const char* pExpression,  /*nullable*/ CScript const* script);
	bool					ConditionHasDefinedType(const char* pCondition, conditionalType_e type);
	void					Set(const char* pName, bool bSet, conditionalType_e type, /*nullable*/ CScript const* script);
	bool					IsDefined(const char* pName);

	void					SetupDefaultConditionals();
	CUtlString				GetCRCStringFromConditionals();


	struct ConditionalTypePredicate
	{
		bool operator () (conditional_t* cond) const { return cond->m_Type == type; }

		conditionalType_e type;
	};

	struct DefinedConditionalTypePredicate
	{
		bool operator () (conditional_t* cond) const { return cond->m_Type == type && cond->m_bDefined; }

		conditionalType_e type;
	};

	using GetAllRange = std::ranges::filter_view<std::ranges::ref_view<CUtlVector<conditional_t*>>, ConditionalTypePredicate>;
	using GetAllDefinedRange = std::ranges::filter_view<std::ranges::ref_view<CUtlVector<conditional_t*>>, DefinedConditionalTypePredicate>;

	GetAllRange GetAll(conditionalType_e type);
	GetAllDefinedRange GetAllDefined(conditionalType_e type);

	Storage const& GetStorage() const;
	bool HasAny() const;

private:
	Storage	_conditionals;

};