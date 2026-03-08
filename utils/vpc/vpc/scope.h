//
// Purpose: Scope - a (usually hierarchical) container of conditionals and macros.
//

#pragma once
#include "utlstring.h"
#include "utlvector.h"

#include "logging.h"


class CConditional2 {
public:
	enum EValue: byte
	{
		v_false = 0,
		v_true = 1,
		v_undefined = 2 // Forcibly undefine conditional in this scope
	};

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
	
	virtual CConditional2* GetOrCreateLocalConditional(char const* name, bool defaultValue) const = 0;
	
	virtual CConditional2* SetConditional(char const* name, bool value = true) = 0;
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
	
	virtual CMacro2* GetOrCreateLocalMacro(char const* name, char const* defaultValue) = 0;
	
	// If value has macro expressions (value = "Example $OUTBINNAME"), they will not be evaluated.
	virtual CMacro2* SetMacro(char const* name, char const* value) = 0;
	virtual void UndefineMacro(char const* name) = 0;
	
	// 'panicOnError': if true, on errors outputs fatal error message and terminates the program.
	// If false, returns false.
	//
	// Successful evaluation returns true.
	virtual bool EvaluateMacroExpression(char const* expr, CUtlStringBuilder& out, bool panicOnError = true)  = 0;
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
	void DumpState() const override = 0; // To stop DumpState(DumpStateInfo const&) hiding the virtual function.
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
	void DumpConditionalOverwrite(CConditional2 const* newState) const;
	void DumpMacroOverwrite(CMacro2 const* newState) const;
	void DumpStateHierarchy(CUtlVector<DumpStateInfo> const& states) const;
	void DumpState(DumpStateInfo const& state) const;

private:
	CUtlString _name;
	bool _dumpOverwrites = false;

protected:
	CDebugContext _debugCtx;
};