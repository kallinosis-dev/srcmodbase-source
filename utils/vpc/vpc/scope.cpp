#include "scope.h"

#include <ranges>

#include "exprevaluator.h"
#include "scriptutil.h"

//--------------------------------------------------------------------------------
// CConditional2

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

	SetValue(value);
}

char const* CConditional2::GetName() const
{
	return _name.Get();
}

size_t CConditional2::GetNameLength() const
{
	return _name.Length();
}

CConditional2::EValue CConditional2::GetValue() const
{
	return _value;
}

void CConditional2::SetValue(EValue value)
{
	_value = value;
}


void IScope::SetValue(CConditional2* conditional, CConditional2::EValue value)
{
	conditional->SetValue(value);
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
// CMacro2

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

size_t CMacro2::GetNameLength() const
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


void IScope::SetValue(CMacro2* macro, char const* value)
{
	macro->SetValue(value);
}

bool CMacro2::GetMakePreprocessorDefine() const
{
	return _makePreprocessorDefine;
}


void CMacro2::SetMakePreprocessorDefine(bool value)
{
	_makePreprocessorDefine = value;
}


void IScope::SetMakePreprocessorDefine(CMacro2* macro, bool value)
{
	macro->SetMakePreprocessorDefine(value);
}

//--------------------------------------------------------------------------------
// CBaseScope

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

	CUtlVector<CConditional2 const*> conds;
	GetLocalConditionals(conds);

	CUtlVector<CMacro2 const*> macros;
	GetLocalMacros(macros);

	log.Status("Scope '%s'\n", _name.Get());
	log.SetWriteName(false);

	if(conds.IsEmpty())
		log.Status("No conditionals defined/forcibly undefined");
	else
	{
		log.Status("Conditionals:");
		for( CConditional2 const* cond: conds)
		{
			log.Status("  %s %s%s", 
				cond->GetName(), 
				cond->IsDefined() ? "= " : "", 
				cond->GetStringValue());
		}

		log.Status(""); // Just a newline
	}

	if(macros.IsEmpty())
		log.Status("No macros defined/forcibly undefined");
	else
	{
		log.Status("Macros:");
		for ( CMacro2 const* macro: macros)
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

//--------------------------------------------------------------------------------
// CBaseScope conditional evaluation

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
// CBaseScope macro evaluation

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

//--------------------------------------------------------------------------------
// CSimpleScope


CSimpleScope::CSimpleScope(CScript const* script, char const* name): CBaseScope(script, name)
{
}

CSimpleScope::~CSimpleScope()
{
	_conditionals.PurgeAndDeleteElements();
	_macros.PurgeAndDeleteElements();
}

void CSimpleScope::DumpState() const
{
	DumpLocalState();
}


//--------------------------------------------------------------------------------
// CSimpleScope conditionals

CConditional2 const* CSimpleScope::GetLocalConditional(char const* name) const
{
	if(auto i =  _conditionals.Find(name); _conditionals.IsValidIndex(i))
	{
		return _conditionals.Element(i);
	}

	return nullptr;
}

CConditional2* CSimpleScope::GetLocalConditional(char const* name)
{
	if(auto i =  _conditionals.Find(name); _conditionals.IsValidIndex(i))
	{
		return _conditionals.Element(i);
	}

	return nullptr;
}

CConditional2 const* CSimpleScope::GetConditional(char const* name) const
{
	return GetLocalConditional(name);
}


void CSimpleScope::GetLocalConditionals(CUtlVector<CConditional2 const*>& out) const
{
	out.EnsureCapacity( out.Count() + _conditionals.Count());

	for (CConditional2* cond : 
		_conditionals | std::ranges::views::transform([this](auto i) { return _conditionals.Element(i); }))
	{
		out.AddToTail(cond);
	}
}

void CSimpleScope::GetConditionals(CUtlVector<CConditional2 const*>& out) const
{
	GetLocalConditionals(out);
}


CConditional2* CSimpleScope::GetOrCreateLocalConditional(char const* name, CConditional2::EValue defaultValue, bool* outCreated)
{
	if(CConditional2* cnd = GetLocalConditional(name))
	{
		if(outCreated)
			*outCreated = false;

		return cnd;
	}

	CConditional2* cnd = new CConditional2(name, defaultValue);

	_conditionals.Insert(name, cnd);

	if(outCreated)
		*outCreated = true;

	return cnd;
}

CConditional2* CSimpleScope::SetConditionalImpl(char const* name, CConditional2::EValue value)
{
	CConditional2 const* cndGlobal = GetConditional(name);
	CConditional2::EValue valGlobal = cndGlobal ? cndGlobal->GetValue() : CConditional2::v_undefined;

	if(valGlobal == value)
		return nullptr;

	bool created;
	CConditional2* cnd = GetOrCreateLocalConditional(name, value, &created);

	if(cnd->GetValue() != value)
	{
		SetValue(cnd, value);

		if(!created)
		{
			DumpConditionalOverwrite(cnd);
		}
	}

	return cnd;
}

CConditional2* CSimpleScope::SetConditional(char const* name, bool value)
{
	return SetConditionalImpl(name, value ? CConditional2::v_true : CConditional2::v_false);
}

void CSimpleScope::UndefineConditional(char const* name)
{
	SetConditionalImpl(name, CConditional2::v_undefined);
}


//--------------------------------------------------------------------------------
// CSimpleScope macros


CMacro2 const* CSimpleScope::GetLocalMacro(char const* name) const
{
	if(auto i =  _conditionals.Find(name); _conditionals.IsValidIndex(i))
	{
		return _macros.Element(i);
	}

	return nullptr;
}

CMacro2* CSimpleScope::GetLocalMacro(char const* name)
{
	if(auto i =  _conditionals.Find(name); _conditionals.IsValidIndex(i))
	{
		return _macros.Element(i);
	}

	return nullptr;
}

CMacro2 const* CSimpleScope::GetMacro(char const* name) const
{
	return GetLocalMacro(name);
}

void CSimpleScope::GetLocalMacros(CUtlVector<CMacro2 const*>& out) const
{
	out.EnsureCapacity( out.Count() + _macros.Count());

	for (CMacro2* cond : 
		_macros | std::ranges::views::transform([this](auto i) { return _macros.Element(i); }))
	{
		out.AddToTail(cond);
	}
}

void CSimpleScope::GetMacros(CUtlVector<CMacro2 const*>& out) const
{
	GetLocalMacros(out);
}


CMacro2* CSimpleScope::GetOrCreateLocalMacro(char const* name, char const* defaultValue, bool* outCreated)
{
	if(CMacro2* macro = GetLocalMacro(name))
	{
		if(outCreated)
			*outCreated = false;

		return macro;
	}

	CMacro2* macro = new CMacro2 (name, defaultValue);

	_macros.Insert(name, macro);

	if(outCreated)
		*outCreated = true;

	return macro;
}

static bool streq_nullable(char const* s1, char const* s2)
{
	if(s1 == s2) 
		return true;

	return s1 && s2 && V_strcmp(s1, s2) == 0;
}

CMacro2* CSimpleScope::SetMacroImpl(char const* name, char const* value)
{
	CMacro2 const* macroGlobal = GetMacro(name);
	char const* valGlobal = macroGlobal ? macroGlobal->GetValue() : nullptr;

	if(streq_nullable(valGlobal, value))
		return nullptr;

	bool created;
	CMacro2* macro = GetOrCreateLocalMacro(name, value, &created);

	if(!streq_nullable(macro->GetValue(),value))
	{
		SetValue(macro, value);

		if(!created)
		{
			DumpMacroOverwrite(macro);
		}
	}

	return macro;
}

CMacro2* CSimpleScope::SetMacro(char const* name, char const* value)
{
	return SetMacroImpl(name, value);
}

void CSimpleScope::UndefineMacro(char const* name)
{
	SetMacroImpl(name, nullptr);
}

//--------------------------------------------------------------------------------
// CInheritedScope

CInheritedScope::CInheritedScope(IScope const* parent, CScript const* script, char const* name):
	CSimpleScope(script, name), _parent(parent)
{
}

CConditional2 const* CInheritedScope::GetConditional(char const* name) const
{
	if(CConditional2 const* cnd = GetLocalConditional(name))
		return cnd;

	return _parent->GetConditional(name);
}

void CInheritedScope::GetConditionals(CUtlVector<CConditional2 const*>& out) const
{
	GetLocalConditionals(out);
	_parent->GetConditionals(out);

}

CMacro2 const* CInheritedScope::GetMacro(char const* name) const
{
	if(CMacro2 const* macro = GetLocalMacro(name))
		return macro;

	return _parent->GetMacro(name);
}


void CInheritedScope::GetMacros(CUtlVector<CMacro2 const*>& out) const
{
	GetLocalMacros(out);
	_parent->GetMacros(out);
}


// ----
// Related utility functions

// O(things.Count() ^ 2) comparasions, each comparasion is O(name_length)
template<typename T>
void DedupliateThing(CUtlVector<T const*>& things, bool removeUndefined)
{
	CUtlVector<T const*> result(0, things.Count());

	for (int i = things.Count() - 1; i >= 0; --i)
	{
		T const* ithing = things[i];
		if(!ithing->IsDefined() && removeUndefined)
		{
			continue;
		}

		auto thingEqualName = [ithing](T const* findmac) -> bool
		{
			if(ithing->GetNameLength() != findmac->GetNameLength()) 
				return false;
			return streq_nullable(findmac->GetName(), ithing->GetName());
		};

		if(!std::ranges::none_of(result, thingEqualName))
		{
			result.AddToTail(ithing);
		}
	}

	things = result;
}


void DeduplicateMacros(CUtlVector<CMacro2 const*>& macros, bool removeUndefined)
{
	DedupliateThing<CMacro2>(macros, removeUndefined);
}

void DeduplicateConditionals(CUtlVector<CConditional2 const*>& conds, bool removeUndefined)
{
	DedupliateThing<CConditional2>(conds, removeUndefined);
}

// End related utility functions
// ----

void CInheritedScope::DumpState() const
{
	DumpLocalState();
	_parent->DumpState();
}
