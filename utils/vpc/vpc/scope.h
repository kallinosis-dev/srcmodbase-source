#pragma once
#include "utlstring.h"
#include "utlvector.h"

class CDebugContext;

struct CConditional2 {
	CUtlString name;
	CUtlString name_uppercase;
	bool value;
};

struct CMacro2 {
	CUtlString name; // Prefixed with $
	CUtlString value;
	
	bool make_define;
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
	
	// IDK if extraCtx is actually needed
	virtual bool EvaluateConditionalExpression(char const* expr, CDebugContext const* extraCtx = nullptr) = 0;
	
// Macros (the VPC macros, different from the MSBuild macros)
	virtual CMacro2 /*nullable*/ const* GetMacro(char const* name) const = 0;
	
	virtual CMacro2 /*nullable*/ const* GetLocalMacro(char const* name) const = 0;
	virtual CMacro2 /*nullable*/ * GetLocalMacro(char const* name) = 0;
	
	virtual void GetMacros(CUtlVector<CMacro2 const*>& out) const = 0;
	virtual void GetLocalMacros(CUtlVector<CMacro2 const*>& out) const = 0;
	
	virtual CMacro2* GetOrCreateLocalMacro(char const* name, char const* defaultValue) = 0;
	
	// If value has macro expressions (value = "Example $OUTBINNAME"), they will not be evaluated.
	virtual CMacro2* SetMacro(char const* name, char const* value) = 0;
	virtual void UndefineMacro(char const* name) = 0;
	
	// If 'expr' contains undefined macro references, it is considered to be an error.
	// If 'errorOnUndefinedMacro' is true, this funciton will output a fancy fatal error message (which terminates the program).
	// If it is false, the function just returns nullptr.
	//
	// Successfull evaluation never returns nullptr.
	virtual char /*maybe nullable*/ const* EvaluateMacroExpression(
		char const* expr, bool errorOnUndefinedMacro = true, CDebugContext const* extraCtx = nullptr) = 0;
};