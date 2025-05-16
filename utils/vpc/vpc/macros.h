#pragma once
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"


struct CMacro;

class CDefCaselessCUtlStringLess
{
public:
	CDefCaselessCUtlStringLess() {}
	CDefCaselessCUtlStringLess(int i) {}
	inline bool operator()(const CUtlString& lhs, const CUtlString& rhs) const { return (V_stricmp_fast(lhs.String(), rhs.String()) < 0); }
	inline bool operator!() const { return false; }
};

using MacroResolveFn = void (*)(CMacro*);

#define MAX_MACRO_NAME 200
struct CMacro
{
public:
	CMacro( const char *pMacroName, const char *pMacroValue, const char *pConfigurationName, bool bSystemMacro, bool bSetupDefine );
	CMacro( const char *pMacroName, MacroResolveFn pFNResolveValue );

	bool IsSystemMacro() const { return m_bSystemMacro; }
	bool IsPropertyMacro() const { return !m_ConfigurationName.IsEmpty(); }
	bool ShouldDefineInProjectFile() const { return m_bSetupDefineInProjectFile; }

    // GetName returns the name without a leading $.
	const char *GetName() const { return m_FullName.Get() + 1; }
    const char *GetFullName() const { return m_FullName.Get(); }
    int GetNameLength() const { return m_nBaseNameLength; }
    int GetFullNameLength() const { return m_nBaseNameLength + 1; }

	const char *GetValue() const
	{
		const_cast<CMacro*>(this)->ResolveValue();
		return m_Value;
	}
    int GetValueLength() const
	{
		const_cast<CMacro*>(this)->ResolveValue();
		return m_Value.Length();
	}
    bool HasValue() const
	{
		const_cast<CMacro*>(this)->ResolveValue();
		return !m_Value.IsEmpty();
	}

	const char *GetConfigurationName() const { return m_ConfigurationName; }
    bool HasConfigurationName() const { return !m_ConfigurationName.IsEmpty(); }

	void SetValue( const char *pMacroValue ) { Assert( !m_pFNResolveDynamicMacro); m_Value = pMacroValue; }
	void SetResolveFunc( MacroResolveFn pFNResolveValue) { m_pFNResolveDynamicMacro = pFNResolveValue; }

private:
	void SetMacroName( const char *pMacroName );
	void ResolveValue( void ) { if ( m_pFNResolveDynamicMacro ) m_pFNResolveDynamicMacro( this ); }

	// m_FullName has the case-preserved macro name with a leading $.
	CUtlString	m_FullName;

protected:
	CUtlString	m_Value;

	// when set denotes the configuration this macro belongs to
	CUtlString	m_ConfigurationName;

    int			m_nBaseNameLength;
    
	// If set to true, then VPC will add this as a -Dname=value parameter to the compiler's command line.
	bool		m_bSetupDefineInProjectFile;
	
	// VPC created this macro itself rather than the macro being created from a script file.
	// System macros are Read-Only to scripts.
	bool		m_bSystemMacro;

	//when set, the macro calls the resolve function whenever HasValue() or GetValue() are called
	MacroResolveFn m_pFNResolveDynamicMacro;
};

class CMacroStorage
{
public:
	using MacroIdx = int;
	// using UtlMap to support duplicates
	using Storage = CUtlMap< CUtlString, CMacro*, MacroIdx, CDefCaselessCUtlStringLess >;

	void					ResolveString(char const* pString, CUtlStringBuilder* pOutBuff, CUtlVector< CUtlString >* pMacrosReplaced = nullptr);
	int						GetMacrosMarkedForCompilerDefines(CUtlVector< CMacro* >& macroDefines);
	void					RemoveScriptCreated();
	const char* GetValue(const char* pMacroName, const char* pConfigurationName = nullptr);
	CMacro* Get(const char* pMacroName, const char* pConfigurationName = nullptr);
	CMacro* SetAsSystem(const char* pMacroName, const char* pMacroValue, bool bSetupDefineInProjectFile = false);
	CMacro* SetAsDynamic(const char* pMacroName, MacroResolveFn pFNResolveValue);
	CMacro* SetAsScript(const char* pMacroName, const char* pMacroValue, bool bSetupDefineInProjectFile = false);
	CMacro* SetAsProperty(const char* pMacroName, const char* pMacroValue, const char* pConfigurationName);

	void GetPreprocessorDefines(char const* cfg_string, CUtlVector<CUtlString>& outDefines) const;

	//using const_iterator = Storage::const_iterator;

	//const_iterator begin() const { return macros.begin(); }
	//const_iterator end() const { return macros.end(); }

	Storage const& GetStorage() const { return _macros; }
		 

private:
	Storage	_macros;
};