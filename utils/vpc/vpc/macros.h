#pragma once
#include "utlstring.h"


#define MAX_MACRO_NAME 200
struct CMacro
{
public:
	CMacro( const char *pMacroName, const char *pMacroValue, const char *pConfigurationName, bool bSystemMacro, bool bSetupDefine );
	CMacro( const char *pMacroName, void (*pFNResolveValue)( CMacro * ) );

	bool IsSystemMacro() const { return m_bSystemMacro; }
	bool IsPropertyMacro() const { return !m_ConfigurationName.IsEmpty(); }
	bool ShouldDefineInProjectFile() const { return m_bSetupDefineInProjectFile; }

    // GetName returns the name without a leading $.
	const char *GetName() { return m_FullName.Get() + 1; }
    const char *GetFullName() { return m_FullName.Get(); }
    int GetNameLength() const { return m_nBaseNameLength; }
    int GetFullNameLength() const { return m_nBaseNameLength + 1; }
	const char *GetValue() { ResolveValue(); return m_Value; }
    int GetValueLength() { ResolveValue(); return m_Value.Length(); }
    bool HasValue() { ResolveValue(); return !m_Value.IsEmpty(); }
	const char *GetConfigurationName() { return m_ConfigurationName; }
    bool HasConfigurationName() const { return !m_ConfigurationName.IsEmpty(); }
	void SetValue( const char *pMacroValue ) { Assert( !m_pFNResolveDynamicMacro); m_Value = pMacroValue; }
	void SetResolveFunc( void (*pFNResolveValue)( CMacro *pThis ) ) { m_pFNResolveDynamicMacro = pFNResolveValue; }

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
	void (*m_pFNResolveDynamicMacro)( CMacro *pThis );
};