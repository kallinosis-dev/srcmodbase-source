//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#pragma once

class CProjectConfiguration;
class CVCProjGenerator;
class CProjectTool;

struct PropertyState_t
{
	ToolProperty_t	*m_pToolProperty;
	CUtlString		m_OrdinalString;
	CUtlString		m_StringValue;
};

// ps3 visual studio integration
enum PS3VSIType_e
{
	PS3_VSI_TYPE_UNDEFINED = -1,
	PS3_VSI_TYPE_SNC = 0,
	PS3_VSI_TYPE_GCC = 1,
};

class CProjectFile
{
public:
	CProjectFile( CVCProjGenerator *pGenerator, const char *pFilename, VpcFileFlags_t iFlags );
	~CProjectFile();

	bool GetConfiguration( const char *pConfigName, CProjectConfiguration **ppConfig );
	bool AddConfiguration( const char *pConfigName, CProjectConfiguration **ppConfig );
	bool RemoveConfiguration( CProjectConfiguration *pConfig );

	bool IsExcludedFrom( const char *pConfigName );

	CUtlString m_Name;
	VpcFileFlags_t m_iFlags;
	CVCProjGenerator *m_pGenerator;
	CUtlVector< CProjectConfiguration* > m_Configs;
	uint32	m_nInsertOrder;
	int64 m_nBuildOrderModifier;
    // Arbitrary ID assigned by the generator for its own uses.
    // For example the Xcode generator uses this to track per-file OID values.
    uint64 m_nGeneratorId;
};

class CProjectFolder
{
public:
	CProjectFolder( CVCProjGenerator *pGenerator, const char *pFolderName, VpcFolderFlags_t iFlags );
	~CProjectFolder();

	bool GetFolder( const char *pFolderName, CProjectFolder **pFolder );
	bool AddFolder( const char *pFolderName, VpcFolderFlags_t iFlags, CProjectFolder **pFolder );
	void AddFile( const char *pFilename, VpcFileFlags_t iFlags, CProjectFile **ppFile );
	bool FindFile( const char *pFilename );
	bool RemoveFile( const char *pFilename );

	CUtlString							m_Name;
	VpcFolderFlags_t					m_iFlags;
	CVCProjGenerator					*m_pGenerator;
	CUtlLinkedList< CProjectFolder* >	m_Folders;
	CUtlLinkedList< CProjectFile* >		m_Files;
};

class CPropertyStateLessFunc
{
public:
	bool Less( const int& lhs, const int& rhs, void *pContext );
};

class CPropertyStates
{
public:
	CPropertyStates();

	bool SetProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
	bool SetBoolProperty( ToolProperty_t *pToolProperty, bool bEnabled );
	const char *GetPropertyValue( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);

	PropertyState_t *GetProperty( int nPropertyId );
	PropertyState_t *GetProperty( const char *pPropertyName );

	CUtlVector< PropertyState_t > m_Properties;
	CUtlSortVector< int, CPropertyStateLessFunc > m_PropertiesInOutputOrder;

private:
	bool SetStringProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
	bool SetListProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
	bool SetBoolProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
	bool SetBoolProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool, bool bEnabled );
	bool SetIntegerProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
};

class CProjectTool
{
public:
	virtual ~CProjectTool() = default;

	CProjectTool( CVCProjGenerator *pGenerator )
	{
		m_pGenerator = pGenerator;
	}

	CVCProjGenerator *GetGenerator() const { return m_pGenerator; }

	// when the property belongs to the root tool (i.e. linker), no root tool is passed in
	// when the property is for the file's specific configuration tool, (i.e. compiler/debug), the root tool must be supplied
	virtual bool SetProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);
	virtual const char *GetPropertyValue( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr);

	CPropertyStates	m_PropertyStates;

private:
	CVCProjGenerator *m_pGenerator;
};

class CDebuggingTool : public CProjectTool
{
public:
	CDebuggingTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CCompilerTool : public CProjectTool
{
public:
	CCompilerTool( CVCProjGenerator *pGenerator, const char *pConfigName, bool bIsFileConfig ) : CProjectTool( pGenerator )
	{
		m_ConfigName = pConfigName;
		m_bIsFileConfig = bIsFileConfig;
	}

	bool SetProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;
	const char *GetPropertyValue( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;

private:
	CUtlString	m_ConfigName;
	bool		m_bIsFileConfig;
};

class CLibrarianTool : public CProjectTool
{
public:
	CLibrarianTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CLinkerTool : public CProjectTool
{
public:
	CLinkerTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CManifestTool : public CProjectTool
{
public:
	CManifestTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CXMLDocGenTool : public CProjectTool
{
public:
	CXMLDocGenTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CBrowseInfoTool : public CProjectTool
{
public:
	CBrowseInfoTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CResourcesTool : public CProjectTool
{
public:
	CResourcesTool( CVCProjGenerator *pGenerator, const char *pConfigName, bool bIsFileConfig ) : CProjectTool( pGenerator )
	{
		m_ConfigName = pConfigName;
		m_bIsFileConfig = bIsFileConfig;
	}

	bool SetProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;
	const char *GetPropertyValue( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;

private:
	CUtlString	m_ConfigName;
	bool		m_bIsFileConfig;
};

class CPreBuildEventTool : public CProjectTool
{
public:
	CPreBuildEventTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CPreLinkEventTool : public CProjectTool
{
public:
	CPreLinkEventTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CPostBuildEventTool : public CProjectTool
{
public:
	CPostBuildEventTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CCustomBuildTool : public CProjectTool
{
public:
	CCustomBuildTool( CVCProjGenerator *pGenerator, const char *pConfigName, bool bIsFileConfig ) : CProjectTool( pGenerator )
	{
		m_ConfigName = pConfigName;
		m_bIsFileConfig = bIsFileConfig;
	}

	bool SetProperty( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;
	const char *GetPropertyValue( ToolProperty_t *pToolProperty, CProjectTool *pRootTool = nullptr) override;

private:
	CUtlString	m_ConfigName;
	bool		m_bIsFileConfig;
};

class CXboxImageTool : public CProjectTool
{
public:
	CXboxImageTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CXboxDeploymentTool : public CProjectTool
{
public:
	CXboxDeploymentTool( CVCProjGenerator *pGenerator ) : CProjectTool( pGenerator ) {}
};

class CProjectConfiguration
{
public:
	CProjectConfiguration( CVCProjGenerator *pGenerator, const char *pConfigName, const char *pFilename );
	~CProjectConfiguration();

	CDebuggingTool *GetDebuggingTool() const { return m_pDebuggingTool; }
	CCompilerTool *GetCompilerTool() const { return m_pCompilerTool; }
	CLibrarianTool *GetLibrarianTool() const { return m_pLibrarianTool; }
	CLinkerTool *GetLinkerTool() const { return m_pLinkerTool; }
	CManifestTool *GetManifestTool() const { return m_pManifestTool; }
	CXMLDocGenTool *GetXMLDocGenTool() const { return m_pXMLDocGenTool; }
	CBrowseInfoTool *GetBrowseInfoTool() const { return m_pBrowseInfoTool; }
	CResourcesTool *GetResourcesTool() const { return m_pResourcesTool; }
	CPreBuildEventTool *GetPreBuildEventTool() const { return m_pPreBuildEventTool; }
	CPreLinkEventTool *GetPreLinkEventTool() const { return m_pPreLinkEventTool; }
	CPostBuildEventTool *GetPostBuildEventTool() const { return m_pPostBuildEventTool; }
	CCustomBuildTool *GetCustomBuildTool() const { return m_pCustomBuildTool; }
	CXboxImageTool *GetXboxImageTool() const { return m_pXboxImageTool; }
	CXboxDeploymentTool *GetXboxDeploymentTool() const { return m_pXboxDeploymentTool; }
	CProjectTool *GetIntellisenseTool() const { return m_pIntellisenseTool; }

	bool IsEmpty() const;

	bool SetProperty( ToolProperty_t *pToolProperty );
	const char *GetPropertyValue( ToolProperty_t *pToolProperty );

	CVCProjGenerator *m_pGenerator;

	// type of config, and config's properties
	bool m_bIsFileConfig;

	CUtlString m_Name;
	CUtlString m_LowerCaseName;

	CPropertyStates	m_PropertyStates;

private:
	// the config's tools
	CDebuggingTool				*m_pDebuggingTool;
	CCompilerTool				*m_pCompilerTool;
	CLibrarianTool				*m_pLibrarianTool;
	CLinkerTool					*m_pLinkerTool;
	CManifestTool				*m_pManifestTool;
	CXMLDocGenTool				*m_pXMLDocGenTool;
	CBrowseInfoTool				*m_pBrowseInfoTool;
	CResourcesTool				*m_pResourcesTool;
	CPreBuildEventTool			*m_pPreBuildEventTool;
	CPreLinkEventTool			*m_pPreLinkEventTool;
	CPostBuildEventTool			*m_pPostBuildEventTool;
	CCustomBuildTool			*m_pCustomBuildTool;
	CXboxImageTool				*m_pXboxImageTool;
	CXboxDeploymentTool			*m_pXboxDeploymentTool;
	CProjectTool				*m_pIntellisenseTool;
};

class IVCProjWriter
{
public:
	virtual ~IVCProjWriter() = default;
	virtual bool Save( const char *pOutputFilename ) = 0;
	virtual const char *GetProjectFileExtension() { return nullptr; }
	virtual CVCProjGenerator *GetProjectGenerator() = 0;
};

class CVCProjGenerator : public CBaseProjectDataCollector
{

public:
	typedef CBaseProjectDataCollector BaseClass;
	CVCProjGenerator();

	const char	*GetProjectFileExtension() override;
	void		StartProject() override;
	void		EndProject( bool bSaveData ) override;
	const char *GetProjectName() override;
	void		SetProjectName( const char *pProjectName ) override;
	void		GetAllConfigurationNames( CUtlVector< CUtlString > &configurationNames ) override;
	void		StartConfigurationBlock( const char *pConfigName, bool bFileSpecific ) override;
	void		EndConfigurationBlock() override;
	bool		StartPropertySection( configKeyword_e keyword, bool *pbShouldSkip ) override;
	void		HandleProperty( const char *pProperty, const char *pCustomScriptData ) override;
	const char *GetPropertyValue( const char *pPropertyName ) override;
	void		EndPropertySection( configKeyword_e keyword ) override;
	void		StartFolder( const char *pFolderName, VpcFolderFlags_t iFlags ) override;
	void		EndFolder() override;
	bool		StartFile( const char *pFilename, VpcFileFlags_t iFlags, bool bWarnIfAlreadyExists ) override;
	void		EndFile() override;
	void		FileExcludedFromBuild( bool bExcluded ) override;
	bool		RemoveFile( const char *pFilename ) override;

	void		EnumerateSupportedVPCTargetPlatforms( CUtlVector<CUtlString> &output ) override;
	bool		BuildsForTargetPlatform( const char *szVPCTargetPlatform ) override;
	bool		DeploysForVPCTargetPlatform( const char *szVPCTargetPlatform ) override;
	CUtlString	GetSolutionPlatformAlias( const char *szVPCTargetPlatform, IBaseSolutionGenerator *pSolutionGenerator ) override;

	CGeneratorDefinition	*GetGeneratorDefinition() const { return m_pGeneratorDefinition; }
	void					SetupGeneratorDefinition( const char *pDefinitionName, PropertyName_t *pPropertyNames );

	void				AddProjectWriter( IVCProjWriter *pVCProjWriter );
    void                RemoveLastProjectWriter() { m_VCProjWriters.RemoveMultipleFromTail( 1 ); }

	PS3VSIType_e		GetVSIType() const { return m_VSIType; }

	bool				GetRootConfiguration( const char *pConfigName, CProjectConfiguration **pConfig );
	void				GetAllRootConfigurations( CUtlVector< CProjectConfiguration * > &rootConfigurations ) const;

	CProjectFolder		*GetRootFolder() const { return m_pRootFolder; }

	// returns true if found, false otherwise (bFixSlashes fixes up slashes in the search string first)
	bool				FindFile( const char *pFilename, CProjectFile **pFile, bool bFixSlashes = false );

	void				AddFileToFolder( const char *pFilename, CProjectFolder *pFolder, bool bWarnIfExists, VpcFileFlags_t iFlags, CProjectFile **pFile );

	void				GetAllProjectFiles( CUtlVector< CProjectFile * > &projectFiles );

    // See if the file has a file-specific property.
    bool                HasFilePropertyValue( CProjectFile *pProjectFile, const char *pConfigrationName, configKeyword_e configKeyword, const char *pPropertyName ) const;
	const char			*GetPropertyValueAsString( CProjectFile *pProjectFile, const char *pConfigrationName, configKeyword_e configKeyword, const char *pPropertyName, const char *pDefaultValue = "" );
	bool				GetPropertyValueAsBool( CProjectFile *pProjectFile, const char *pConfigrationName, configKeyword_e configKeyword, const char *pPropertyName, bool bDefaultValue = false );

private:
	void				Clear();
	bool 				Config_GetConfigurations( const char *pszConfigName );

	// returns true if found, false otherwise
	bool				GetFolder( const char *pFolderName, CProjectFolder *pParentFolder, CProjectFolder **pOutFolder ) const;
	// returns true if added, false otherwise (duplicate)
	bool				AddFolder( const char *pFolderName, CProjectFolder *pParentFolder, VpcFolderFlags_t iFlags, CProjectFolder **pOutFolder ) const;

	// returns true if removed, false otherwise (not found)
	bool				RemoveFileFromFolder( const char *pFilename, CProjectFolder *pFolder );

	bool				IsConfigurationNameValid( const char *pConfigName );

	configKeyword_e		SetPS3VisualStudioIntegrationType( configKeyword_e eKeyword );

	void				ApplyInternalPreprocessorDefinitions();

    void                LogOutputFiles( const char *pConfigName );

	int					FindFileInDictionary( const char *pFilename );

	void				EvaluateHackMacro_HACK_DEPENDENCIES_ALLVPCSCRIPTS( void );

	void				AddIndirectCustomBuildDependencies( void );
    
private:
	configKeyword_e			m_nActivePropertySection;
	CGeneratorDefinition	*m_pGeneratorDefinition;

	CDebuggingTool			*m_pDebuggingTool;
	CCompilerTool			*m_pCompilerTool;
	CLibrarianTool			*m_pLibrarianTool;
	CLinkerTool				*m_pLinkerTool;
	CManifestTool			*m_pManifestTool;
	CXMLDocGenTool			*m_pXMLDocGenTool;
	CBrowseInfoTool			*m_pBrowseInfoTool;
	CResourcesTool			*m_pResourcesTool;
	CPreBuildEventTool		*m_pPreBuildEventTool;
	CPreLinkEventTool		*m_pPreLinkEventTool;
	CPostBuildEventTool		*m_pPostBuildEventTool;
	CCustomBuildTool		*m_pCustomBuildTool;
	CXboxImageTool			*m_pXboxImageTool;
	CXboxDeploymentTool		*m_pXboxDeploymentTool;
	CProjectTool			*m_pIntellisenseTool;

	CProjectConfiguration	*m_pConfig;
	CProjectConfiguration	*m_pFileConfig;
	CProjectFile			*m_pProjectFile;

	CSimplePointerStack< CProjectFolder*, CProjectFolder*, 128 >		m_spFolderStack;
	CSimplePointerStack< CCompilerTool*, CCompilerTool*, 128 >			m_spCompilerStack;
	CSimplePointerStack< CCustomBuildTool*, CCustomBuildTool*, 128 >	m_spCustomBuildToolStack;
	CSimplePointerStack< CResourcesTool*, CResourcesTool*, 128 >		m_spResourcesToolStack;

	CUtlString				m_ProjectName;

	CProjectFolder			*m_pRootFolder;

	CUtlVector< CProjectConfiguration* >	m_RootConfigurations;

	// primary file dictionary
	CUtlRBTree< CProjectFile*, int >	m_FileDictionary;

	CUtlVector< IVCProjWriter* >			m_VCProjWriters;

	// ps3 visual studio integration
	PS3VSIType_e			m_VSIType;
};
