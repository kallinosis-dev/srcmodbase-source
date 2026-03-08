#include "scope.h"

#include "exprevaluator.h"
#include "scriptutil.h"

char const* CConditional2::EValueToString(EValue val)
{
	switch (val)
	{
	case v_false:
		return "false";
	case v_true:
		return "true";
	case v_undefined:
		return "undefined";
	default:
		AssertMsg1(false, "Invalid value %ihh", (signed char)val);
		return "<error>";
	}
}

CConditional2::CConditional2(char const* name, EValue value)
{
	_name = name;
	_name_uppercase = _name;
	_name_uppercase.ToUpper();

	SetValue(value);
}

char const* CConditional2::GetName() const
{
	return _name.Get();
}

char const* CConditional2::GetNameUppercase() const
{
	return _name_uppercase.Get();
}

CConditional2::EValue CConditional2::GetValue() const
{
	return _value;
}

void CConditional2::SetValue(EValue value)
{
	_value = value;
}

char const* CConditional2::GetStringValue() const
{
	return EValueToString(_value);
}

bool CConditional2::IsSet() const
{
	return _value == v_true;
}

bool CConditional2::IsDefined() const
{
	return _value != v_undefined;
}


//--------------------------------------------------------------------------------

static bool IsValidMacroNameChar( char ch )
{
    return ch == '_' || V_isalnum( ch );
}


CMacro2::CMacro2(char const* fullname, char const* value)
{
	_name = fullname;
	Assert(_name.Length() < MAX_MACRO_NAME);

	SetValue(value);
}

char const* CMacro2::GetFullName() const
{
	return _name.Get();
}

char const* CMacro2::GetName() const
{
	return _name.Get() + 1;
}

size_t CMacro2::GetFullNameLength() const
{
	return _name.Length() + 1;
}

size_t CMacro2::getNameLength() const
{
	return _name.Length();
}

char const* CMacro2::GetValue() const
{
	if(_defined)
		return _value.Get();
	else
		return nullptr;
}

size_t CMacro2::GetValueLength() const
{
	return _value.Length();
}

bool CMacro2::IsDefined() const
{
	return _defined;
}

void CMacro2::SetValue(char const* value)
{
	_defined = value != nullptr;
	if(_defined)
	{
		_value = value;
	}
	else
	{
		_value.Clear();
	}
}

bool CMacro2::GetMakePreprocessorDefine() const
{
	return _makePreprocessorDefine;
}


void CMacro2::SetMakePreprocessorDefine(bool value)
{
	_makePreprocessorDefine = value;
}


//--------------------------------------------------------------------------------

CBaseScope::CBaseScope(CScript const* script, char const* name):
	_name(name),
	_debugCtx{ .name = _name.Get(), .script = script, .scope = this }
{
}

CDebugContext const* CBaseScope::GetDebugCtx() const
{
	return &_debugCtx;
}

char const* CBaseScope::GetName() const
{
	return _name.Get();
}

void CBaseScope::DumpLocalState() const
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	DumpStateInfo ds;
	ds.Name = _name.Get();
	GetLocalConditionals(ds.Conditionals);
	GetLocalMacros(ds.Macros);
	
	log.Status("Local scope dump:");
	DumpState(ds);
}

void CBaseScope::SetDumpOverwrites(bool value)
{
	_dumpOverwrites = value;
}


bool CBaseScope::GetDumpOverwrites() const
{
	return _dumpOverwrites;
}

void CBaseScope::DumpConditionalOverwrite(CConditional2 const* newState) const
{
	if(!_dumpOverwrites) return;
	MAKE_CONTEXTUAL_LOGGER_AUTO;
	log.SetWriteScriptPosition(true);

	if(newState->IsDefined())
		log.Status("Conditional '%s' overwritten to %s", newState->GetName(), newState->GetStringValue());
	else
		log.Status("Conditional '%s' forcibly undefined", newState->GetName());
}

void CBaseScope::DumpMacroOverwrite(CMacro2 const* newState) const
{
	if(!_dumpOverwrites) return;
	MAKE_CONTEXTUAL_LOGGER_AUTO;
	log.SetWriteScriptPosition(true);

	if(newState->IsDefined())
		log.Status("Macro '%s' overwritten to '%s'", newState->GetName(), newState->GetValue());
	else
		log.Status("Macro '%s' forcibly undefined", newState->GetName());
}

void CBaseScope::DumpStateHierarchy(CUtlVector<DumpStateInfo> const& states) const
{
	for (DumpStateInfo const& state : states)
	{
		DumpState(state);
	}
}

void CBaseScope::DumpState(DumpStateInfo const& state) const
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	log.Status("Scope '%s'\n", state.Name);
	log.SetWriteName(false);

	if(state.Conditionals.IsEmpty())
		log.Status("No conditionals defined/forcibly undefined");
	else
	{
		log.Status("Conditionals:");
		for( CConditional2 const* cond: state.Conditionals)
		{
			log.Status("  %s %s%s", 
				cond->GetName(), 
				cond->IsDefined() ? "= " : "", 
				cond->GetStringValue());
		}

		log.Status(""); // Just a newline
	}

	if(state.Macros.IsEmpty())
		log.Status("No macros defined/forcibly undefined");
	else
	{
		log.Status("Macros:");
		for ( CMacro2 const* macro: state.Macros)
		{
			char const* define_postfix = macro->GetMakePreprocessorDefine() ? " (#define)" : "";

			if(macro->IsDefined())
				log.Status("  %s = \"%s\"%s", macro->GetName(), macro->GetValue(), define_postfix);
			else
				log.Status("  %s undefined%s", macro->GetName(), define_postfix);
		}

		log.Status(""); // Another newline
	}

}

//--------------------------------------------------------------------------------

struct EvaluateCondExprCtx
{
	CBaseScope const* scope;
	bool errorIfUndefined;
};

bool CBaseScope::EvaluateConditionalExpression(char const* expr, bool errorIfUndefined)
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	EvaluateCondExprCtx ctx { this, errorIfUndefined};

	bool result;

	CExpressionEvaluator evaluator;
	bool ok = evaluator.Evaluate(result, expr, 
		[](char const* symbol, void* ctx)
	{
		EvaluateCondExprCtx ctx2 = *((EvaluateCondExprCtx*)ctx);

		return ctx2.scope->ResolveConditionalSymbol(symbol, ctx2.errorIfUndefined);
	}, 
		[](char const* errorMsg, void* ctx)
	{
		EvaluateCondExprCtx ctx2 = *((EvaluateCondExprCtx*)ctx);
		MAKE_CONTEXTUAL_LOGGER(&ctx2.scope->_debugCtx);

		log.Error("Conditional evaluation error: %s", errorMsg);
	}, &ctx);

	if(!ok)
		log.Error("Error evaluating conditional expression");

	return result;
}

bool CBaseScope::ResolveConditionalSymbol(char const* symbol, bool errorIfUndefined) const
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	if(symbol[0] == '$')
		++symbol;

	if(MaybeBool mb = Script_ParseMaybeBool(symbol); mb != MaybeBool::Unknown)
	{
		return mb == MaybeBool::True;
	}

	if(CConditional2 const* cond = GetConditional(symbol))
	{
		if(errorIfUndefined && !cond->IsDefined())
			log.Error("Conditional $%s was forcibly undefined.", cond->GetName());

		return cond->IsSet();
	}

	if(CMacro2 const* macro = GetMacro(symbol); macro->GetValue() != nullptr)
	{
		// MACROS ARE NOT ALLOWED IN CONDITIONALS because it became too commonplace to use the wrong symbol
		// causing quiet unintended results and since macros can be arbitrary strings, their placement in a
		// conditional expression is invalid. Restricting to conditionals ensures we are only ever evaluating
		// is valid boolean values.

		log.Error("Macro $%s found in conditional expression. This is not allowed. Use \"$Conditional <name> <0/1>\" to set conditionals.",
			macro->GetName());
	}

	if(errorIfUndefined)
		log.Error("$%s was never defined as a conditional", symbol);

	// Never-defined expression, assumed to be false
	return false;
}


//--------------------------------------------------------------------------------

bool CBaseScope::EvaluateMacroExpression(char const* expr, CUtlStringBuilder& out, bool panicOnError)
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	out.Set(expr);

	for(size_t i = 0; i < out.Length();)
	{
		char const* macroStart = strchr(out.Get() + i, '$'); // Find something looking like macro start.
		if(!macroStart) break;

		++macroStart; // Skip the $

		// If we don't find a macro for this $token we start scanning
        // right after the $. If we do find a macro and replace some
        // text we'll start right where we did the replacement at the $.
		i = macroStart - out.Get();

		if(!IsValidMacroNameChar(*macroStart))
			continue; // Filters out $(MSBuildMacros) and ${MakefileMacros}

		if(CMacro2 const* macro = FindLongestMatchingMacro(macroStart))
		{
			if (out.ReplaceAt(i - 1, macro->GetFullNameLength(), macro->GetValue(), macro->GetValueLength()))
			{
				// We replaced the text starting from the $ so restart
				// the scan there to pick up any new macro that came in.
				--i;
			}
		}
		else 
		{
			if(panicOnError)
				log.Error("No defined macro found matching %s", macroStart - 1);
			else
				return false;
		}
	}

	return true;
}

//
// Find longest-named macro matching substring from start to afterEnd.
//
//                          V--afterEnd
// E.g. "Blahblah $FOOBAR123 $NEXT_MACRO\0"
//                 ^--start  
//
// Possible matches: macro FOO, macro FOOBAR.
// Longest possible match: macro FOOBAR.
//
CMacro2 const* CBaseScope::FindLongestMatchingMacro(char const* start) const
{
	char const* end = start;
	while(IsValidMacroNameChar(*end))
	{
		end++;
	}

	char const* afterEnd = end + 1;

	size_t len =afterEnd - start; // Here len includes \0
	if(len > MAX_MACRO_NAME)
	{
		len = MAX_MACRO_NAME;
		afterEnd = start + MAX_MACRO_NAME;
	}

	char buf[MAX_MACRO_NAME];
	V_strncpy(buf, start, len);

	do
	{
		len--;

		CMacro2 const* macro = GetMacro(buf);
		if(macro && macro->IsDefined())
		{
			return macro;
		}

		buf[len - 1] = '\0';
	}
	while (len != 0);

	return nullptr;
}