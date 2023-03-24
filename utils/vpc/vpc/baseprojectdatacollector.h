//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================

#pragma once

#include "tier1/keyvalues.h"
#include "tier1/utlstack.h"

class CSpecificConfig
{
public:
	CSpecificConfig( CSpecificConfig *pParentConfig );
	~CSpecificConfig();

	const char	*GetConfigName();
	const char	*GetOption( const char *pOptionName, const char *pDefaultValue = nullptr );

public:
	CSpecificConfig *m_pParentConfig;
	KeyValues		*m_pKV;
	bool			m_bFileExcluded;	// Is the file that holds this config excluded from the build?
};

class CFileConfig
{
public:
	CFileConfig( const char *pFilename, VpcFileFlags_t iFlags ) : m_Filename( pFilename ), m_iFlags( iFlags ) {}
	~CFileConfig();

	void			Term();
	const char		*GetName();
	CSpecificConfig	*GetConfig( const char *pConfigName );
	CSpecificConfig	*GetOrCreateConfig( const char *pConfigName, CSpecificConfig *pParentConfig );
	bool			IsExcludedFrom( const char *pConfigName );

public:
	CUtlDict< CSpecificConfig*, int >	m_Configurations;
	CUtlString							m_Filename;	// "" if this is the config data for the whole project.
	VpcFileFlags_t						m_iFlags;
};

// This just holds the list of property names that we're supposed to scan for.
class CRelevantPropertyNames
{
public:
	const char **m_pNames;
	int			m_nNames;
};

//
// This class is shared by the makefile and SlickEdit project file generator.
// It just collects interesting file properties into KeyValues and then the project file generator
// is responsible for using that data to write out a project file.
//
class CBaseProjectDataCollector : public IBaseProjectGenerator
{
// IBaseProjectGenerator implementation.
public:

	CBaseProjectDataCollector( CRelevantPropertyNames *pNames );
	~CBaseProjectDataCollector();

	// Called before doing anything in a project
void StartProject() OVERRIDE;
void EndProject( bool bSaveData ) OVERRIDE;

	// Access the project name.
const char *GetProjectName() OVERRIDE;
void SetProjectName( const char *pProjectName ) OVERRIDE;

	// Get a list of all configurations.
void GetAllConfigurationNames( CUtlVector< CUtlString > &configurationNames ) OVERRIDE;

	// Configuration data is specified in between these calls and inside BeginPropertySection/EndPropertySection.
	// If bFileSpecific is set, then the configuration data only applies to the last file added.
void StartConfigurationBlock( const char *pConfigName, bool bFileSpecific ) OVERRIDE;
void EndConfigurationBlock() OVERRIDE;

	// Get the current configuration name. Only valid between StartConfigurationBlock()/EndConfigurationBlock()
const char *GetCurrentConfigurationName() OVERRIDE;

	// These functions are called when it enters a section like $Compiler, $Linker, etc.
	// In between the BeginPropertySection/EndPropertySection, it'll call HandleProperty for any properties inside that section.
	// 
	// If you pass pCustomScriptData to HandleProperty, it won't touch the global parsing state -
	// it'll parse the platform filters and property value from pCustomScriptData instead.
bool StartPropertySection( configKeyword_e keyword, bool *pbShouldSkip = NULL ) OVERRIDE;
void HandleProperty( const char *pProperty, const char *pCustomScriptData = NULL ) OVERRIDE;
const char *GetPropertyValue( const char *pProperty ) OVERRIDE;
void EndPropertySection( configKeyword_e keyword ) OVERRIDE;

	// Files go in folders. The generator should maintain a stack of folders as they're added.
void StartFolder( const char *pFolderName, VpcFolderFlags_t iFlags ) OVERRIDE;
void EndFolder() OVERRIDE;
	
	// Add files. Any config blocks/properties between StartFile/EndFile apply to this file only.
	// It will only ever have one active file.
bool StartFile( const char *pFilename, VpcFileFlags_t iFlags, bool bWarnIfAlreadyExists ) OVERRIDE;
void EndFile() OVERRIDE;

	// This is actually just per-file configuration data.
void FileExcludedFromBuild( bool bExcluded ) OVERRIDE;

	// Remove the specified file.
bool RemoveFile( const char *pFilename ) OVERRIDE;		// returns ture if a file was removed

bool HasFile( const char *pFilename ) OVERRIDE
    {
        return m_Files.HasElement( pFilename );
    }

const char *GetCurrentFileName() OVERRIDE;

public:
	void Term();
	static void DoStandardVisualStudioReplacements( const char *pInitStr, CUtlStringBuilder *pStr, const char *pFullInputFilename );
	static void DoShellScriptReplacements( CUtlStringBuilder *pStr );
	static void DoBatchScriptReplacements( CUtlStringBuilder *pStr );

public:
	CUtlString							m_ProjectName;

	CUtlDict< CFileConfig *, int >		m_Files;
	CFileConfig							m_BaseConfigData;

	CUtlStack< CFileConfig* >			m_CurFileConfig;			// Either m_BaseConfigData or one of the files.
	CUtlStack< CSpecificConfig* >		m_CurSpecificConfig;		// Debug, release?

	CRelevantPropertyNames				m_RelevantPropertyNames;
};
