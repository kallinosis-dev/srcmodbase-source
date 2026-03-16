//
// Purpose: Scope - a (usually hierarchical) container of conditionals and macros.
//

#pragma once
#include "utlstring.h"
#include "utlvector.h"

#include "logging.h"
#include "utlmap.h"

class IScope;

class CConditional2 {
	friend IScope;

public:
	enum class EValue: byte
	{
		v_false = 0,
		v_true = 1,
		v_undefined = 2 // Forcibly undefine conditional in this scope
	};
	using enum EValue;

	static char const* EValueToString(EValue val);


public:
	CConditional2(char const* name, EValue value = v_undefined);

	char const* GetName() const;
	char const* GetNameUppercase() const;

	EValue GetValue() const;
	char const* GetStringValue() const;
	bool IsSet() const;
	bool IsDefined() const;

private:
	void SetValue(EValue value);

private:
	CUtlString _name;
	CUtlString _name_uppercase;
	EValue _value = v_undefined;
};

constexpr size_t MAX_MACRO_NAME = 200; // Including \0

struct CMacro2 {
	friend IScope;

public:
	// 'fullname' should start with $
	CMacro2(char const* fullname, char /*nullable*/ const* value);

	char const* GetFullName() const; // Name, prefixed with $
	char const* GetName() const; // Name without $ prefix

	size_t GetFullNameLength() const;
	size_t getNameLength() const;

	char /*nullable*/ const* GetValue() const;
	size_t GetValueLength() const;
	bool IsDefined() const;

	bool GetMakePreprocessorDefine() const;

private:
	void SetValue(char /*nullable*/ const* value);
	void SetMakePreprocessorDefine(bool value);

private:
	CUtlString _name; // Prefixed with $
	CUtlString _value;

	bool _defined = false;
	bool _makePreprocessorDefine = false;
};

class IScope {
public:
	virtual ~IScope() = default;
	
	virtual CDebugContext const* GetDebugCtx() const = 0;
	virtual char const* GetName() const = 0;
	
	virtual void DumpState() const = 0;
	virtual void DumpLocalState() const = 0;
	virtual void SetDumpOverwrites(bool value = true) = 0;
	
	// On Conditional vs LocalConditional, Macro vs LocalMacro and other cases of such naming:
	// 'Local' means defined in this scope, mutable; non-local means maybe defined here, maybe inherited, immutable.
	
	
// Conditionals
	virtual CConditional2 /*nullable*/ const* GetConditional(char const* name) const = 0;
	
	virtual CConditional2 /*nullable*/ const* GetLocalConditional(char const* name) const = 0;
	virtual CConditional2 /*nullable*/ * GetLocalConditional(char const* name) = 0;
	
	virtual void GetConditionals(CUtlVector<CConditional2 const*>& out) const = 0;
	virtual void GetLocalConditionals(CUtlVector<CConditional2 const*>& out) const = 0;
	
	virtual CConditional2* GetOrCreateLocalConditional(char const* name, CConditional2::EValue defaultValue, bool* outCreated = nullptr) = 0;

	// Returns null if nothing changed.
	virtual CConditional2 /*nullable*/* SetConditional(char const* name, bool value = true) = 0;
	virtual void UndefineConditional(char const* name) = 0;

	// If 'errorIfUndefined' is true and undefined conditional is found, output a fancy fatal error message (which terminates the program).
	// If it is false, assume undefined conditionals are false.
	virtual bool EvaluateConditionalExpression(char const* expr, bool errorIfUndefined = false) = 0;

// Macros
	virtual CMacro2 /*nullable*/ const* GetMacro(char const* name) const = 0;
	
	virtual CMacro2 /*nullable*/ const* GetLocalMacro(char const* name) const = 0;
	virtual CMacro2 /*nullable*/ * GetLocalMacro(char const* name) = 0;
	
	virtual void GetMacros(CUtlVector<CMacro2 const*>& out) const = 0;
	virtual void GetLocalMacros(CUtlVector<CMacro2 const*>& out) const = 0;
	
	virtual CMacro2* GetOrCreateLocalMacro(char const* name, char const* defaultValue, bool* outCreated = nullptr) = 0;
	
	// If value has macro expressions (value = "Example $OUTBINNAME"), they will not be evaluated.
	// Returns null if nothing changed.
	virtual CMacro2 /*nullable*/* SetMacro(char const* name, char const* value) = 0;
	virtual void UndefineMacro(char const* name) = 0;
	
	// 'panicOnError': if true, on errors outputs fatal error message and terminates the program.
	// If false, returns false.
	//
	// Successful evaluation returns true.
	virtual bool EvaluateMacroExpression(char const* expr, CUtlStringBuilder& out, bool panicOnError = true)  = 0;

protected:
	// A way to access otherwise-private value setters.

	static void SetValue(CConditional2* conditional, CConditional2::EValue value);

	static void SetValue(CMacro2* macro, char /*nullable*/ const* value);
	static void SetMakePreprocessorDefine(CMacro2* macro, bool value);
};

//----------------------------------------------------

class CBaseScope : public IScope
{
public:
	// Both script and name are used for debug/logging info
	CBaseScope(CScript /*nullable*/ const* script, char const* name);

	CDebugContext const* GetDebugCtx() const override;
	char const* GetName() const override;

	void DumpLocalState() const override;
	void SetDumpOverwrites(bool value = true) override;



// Conditionals
	bool EvaluateConditionalExpression(char const* expr, bool errorIfUndefined = false) override;

// Macros
	bool EvaluateMacroExpression(char const* expr, CUtlStringBuilder& out, bool panicOnError = true) override;

private:
	bool ResolveConditionalSymbol(char const* symbol, bool errorIfUndefined) const;

	CMacro2 const* FindLongestMatchingMacro(char const* start) const;

protected:
	struct DumpStateInfo
	{
		char const* Name;
		CUtlVector<CConditional2 const*> Conditionals;
		CUtlVector<CMacro2 const*> Macros;
	};

	bool GetDumpOverwrites() const;

	// Call this when conditional defined in *local* scope is overwritten. 
	void DumpConditionalOverwrite(CConditional2 const* newState) const;
	// Call this when macro defined in *local* scope is overwritten.
	void DumpMacroOverwrite(CMacro2 const* newState) const;

	void DumpStateHierarchy(CUtlVector<DumpStateInfo> const& states) const;
	void DumpState_Impl(DumpStateInfo const& state) const;

private:
	CUtlString _name;
	bool _dumpOverwrites = false;

protected:
	CDebugContext _debugCtx;
};

//----------------------------------------------------

class CSimpleScope: public CBaseScope
{
public:
	CSimpleScope(CScript const* script, char const* name);
	~CSimpleScope();

	// !! Override me if you are adding parent scopes !!
	void DumpState() const override;

	// Conditionals

	// !! Override me if you are adding parent scopes !!
	CConditional2 /*nullable*/ const* GetConditional(char const* name) const override;

	CConditional2 /*nullable*/ const* GetLocalConditional(char const* name) const override;
	CConditional2 /*nullable*/ * GetLocalConditional(char const* name) override;

	// !! Override me if you are adding parent scopes !!
	void GetConditionals(CUtlVector<CConditional2 const*>& out) const override;
	void GetLocalConditionals(CUtlVector<CConditional2 const*>& out) const override;

	CConditional2* GetOrCreateLocalConditional(char const* name, CConditional2::EValue defaultValue, bool* outCreated = nullptr) override;

private:
	// Returns null if nothing changed
	CConditional2 /*maybe nullable*/ * SetConditionalImpl(char const* name, CConditional2::EValue value);

public:
	// Returns null if nothing changed.
	CConditional2 /*nullable*/* SetConditional(char const* name, bool value = true) override;
	void UndefineConditional(char const* name) override;

// Macros
	// !! Override me if you are adding parent scopes !!
	CMacro2 /*nullable*/ const* GetMacro(char const* name) const override;

	CMacro2 /*nullable*/ const* GetLocalMacro(char const* name) const override;
	CMacro2 /*nullable*/ * GetLocalMacro(char const* name) override;

	// !! Override me if you are adding parent scopes !!
	void GetMacros(CUtlVector<CMacro2 const*>& out) const override;
	void GetLocalMacros(CUtlVector<CMacro2 const*>& out) const override;

	CMacro2* GetOrCreateLocalMacro(char const* name, char const* defaultValue, bool* outCreated = nullptr) override;

private:
	// Returns null if nothing changed.
	CMacro2 /*nullable*/ * SetMacroImpl(char const* name, char /*nullable*/ const* value);

public:
	// Returns null if nothing changed.
	CMacro2 /*nullable*/ * SetMacro(char const* name, char const* value) override;
	void UndefineMacro(char const* name) override;

private:
	CUtlMap<CUtlString, CMacro2*> _macros;
	CUtlMap<CUtlString, CConditional2*> _conditionals;
};