//========= Copyright � 1996-2016, Valve Corporation, All rights reserved. ============//
//
// Purpose: VPC
//
//=====================================================================================//

#pragma once

#if defined( WIN32 )

#include "tier0/wchartypes.h"

#include "winlite.h"
#include <io.h>

// Allow atlbase.h to access 'CreateEvent' in the windows API (see windows_undefines.h)
#undef  CreateEventA
#include <atlbase.h>
#define CreateEventA  CreateEvent

#endif // WIN32

#include "tier1/utlstring.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "tier1/utlsortvector.h"
#include "tier1/checksum_crc.h"
#include "scriptsource.h"
#include "logging.h"
#include "tier1/strtools.h"
#include "sys_utils.h"
#include "tier1/keyvalues.h"
#include "generatordefinition.h"
#include "macros.h"
#include "tier1/UtlStringMap.h"
#include "conditionals.h"
#include "groupscript.h"
#include "projectcache.h"
#include "projectscript.h"


struct project_t;
struct scriptList_t;
class CVCProjGenerator;
class CProjectConfiguration;
struct PropertyState_t;
class CProjectDependencyGraph;
class CDependency_Project;
class IBaseSolutionGenerator;
class CBaseProjectDataCollector;
class CProjectFile;


// We try to avoid any kind of fixed path buffers
// but we still have certain calls that fill
// in fixed buffers.  Make local defines so that
// we can control things separately from MAX_PATH.
#define MAX_FIXED_PATH 2000
// Maximum size of a bare filename with no path.
#define MAX_BASE_FILENAME 260

//#define DISALLOW_UNITY_FILE_EXCLUSION 1

struct KeywordName_t
{
	const char			*m_pName;
	configKeyword_e		m_Keyword;
};

typedef bool (*GetSymbolProc_t)( const char *pKey );

#define INVALID_INDEX -1



#define k_bVPCForceLowerCase false




class IProjectIterator
{
public:
	virtual ~IProjectIterator() = default;
	// the base IProjectIterator::VisitProject() implementation performs the CRC check,
	// so derived classes should call it if they want to skip up-to-date projects
	// NOTE: iProject/projectIndex_t indexes CVPC::m_Projects/project_t
	virtual bool VisitProject( projectIndex_t iProject, const char *szScriptPath );

	// the base IProjectIterator::VisitProject() operates quietly, it's status is stored here
	CUtlString m_CRCCheckStatusSpew;
};

class CUtlStringCaseLess
{
public:
	bool Less( const CUtlString &lhs, const CUtlString &rhs, void *pCtx )
	{
		return ( V_stricmp_fast( lhs.Get(), rhs.Get() ) < 0 ? true : false );
	}
};


class CVPC
{
public:
	CVPC();
	~CVPC();

	bool		Init(int argc, char const* const* argv);
	void		Shutdown( bool bHasError = false );

	bool		HasCommandLineParameter( const char *pParamName ) const;

	bool		IsQuietValidSpew() const { return m_bQuietValidSpew; }
	void		SetQuietValidSpew( bool bQuiet ){ m_bQuietValidSpew = bQuiet; }
	bool		IsShowDependencies() const { return m_bShowDeps; }
	bool		IsForceGenerate() const { return m_bForceGenerate; }
	bool		IsForceIterate() const { return m_bForceIterate || IsForceGenerate(); }
	bool		IsCheckFiles() const { return m_bCheckFiles; }
	
	bool		IsShowFixedPaths() const { return m_bShowFixedPaths; }
	bool		IsShowCaseIssues() const { return m_bShowCaseIssues; }

	bool		IsSourceControlEnabled() const { return m_bSourceControl; }
	bool		IsOSMacroEnabled() const { return m_bAllowOSMacro; }
	bool		IsCRCCheckInProjectEnabled() const { return m_bCRCCheckInProject; }
	bool		IsMissingFileAsErrorEnabled() const { return m_bMissingFileIsError; }
	bool		IsFilePatternEnabled() const { return m_bAllowFilePattern; }
	bool		AddExecuteableToCRCChecks() const { return m_bAddExecuteableToCRC; }
    bool        IsPerFileCompileConfigEnabled() const { return m_bPerFileCompileConfig; }
    bool		IsLibWithinLibEnabled() const { return m_bAllowLibWithinLib; }
    
	bool		Is2010() const { return m_bUse2010; }
	bool		Is2012() const { return m_bUse2012; }
	bool		Is2013() const { return m_bUse2013; }
	bool		Is2015() const { return m_bUse2015; }
	bool		Is2022() const { return m_bUse2022; }

	bool		PrefersVS2010() const { return m_bPreferVS2010; }
	bool		PrefersVS2012() const { return m_bPreferVS2012; }
	bool		PrefersVS2013() const { return m_bPreferVS2013; }
	bool		PrefersVS2015() const { return m_bPreferVS2015; }
	bool		PrefersVS2022() const { return m_bPreferVS2022; }
  
	bool		IsForceRebuildCache() const { return m_bForceRebuildCache; }
	bool		IsDedicatedBuild() const { return m_bDedicatedBuild; }
	bool		UseValveBinDir() const { return m_bUseValveBinDir; }

	bool		IsQtEnabled();
	bool		IsSchemaEnabled();
	bool		IsUnityEnabled();
	bool		IsProjectUsingUnity( script_t *pProjectScript = nullptr);
	bool		IsClangEnabled();
	bool		ShouldEmitClangProject();

	bool		RestrictProjectsToEverything() const { return m_bRestrictProjects; }

	int			GetMissingFilesCount() const	{ return m_nFilesMissing; }
	int			GetTotalMissingFilesCount() const	{ return m_nTotalFilesMissing; }
	void		IncrementFileMissing()	{ ++m_nFilesMissing; ++m_nTotalFilesMissing; }
	void		ResetMissingFilesCount() { m_nFilesMissing = 0; }


	bool UsingShallowDependencies( void ) const { return m_bShallowDepencies; }

	const char *GetStartDirectory()			{ return m_StartDirectory.Get(); }
	const char *GetSourcePath()				{ return m_SourcePath.Get(); }
	
	const char *GetCRCString()				{ return m_SupplementalCRCString.Get(); }
	const char *GetVSAddInMetadataString()	{ return m_VSAddinMetadata.Get(); }
	const char *GetSolutionItemsFilename()	{ return m_SolutionItemsFilename.Get(); }
	const CUtlVector< CUtlString > &GetSolutionFolderNames() { return m_SolutionFolderNames; }

	void DecorateProjectName( CUtlString &undecoratedName );
	void UndecorateProjectName( CUtlString &decoratedName );

	const char *GetProjectName()							{ return m_ProjectName.Get(); }
	void SetProjectName( const char *pProjectName )			{ m_ProjectName = pProjectName; }

	const char *GetLoadAddressName()						{ return m_LoadAddressName.Get(); }
	void SetLoadAddressName( const char *pLoadAddressName )	{ m_LoadAddressName = pLoadAddressName; }

	const char *GetGameName()								{ return m_GameName.Get(); }
	void SetGameName( const char *pGameName )				{ m_GameName = pGameName; }

	const char *GetOutputMirrorPath()						{ return m_OutputMirrorString.Get(); }

	const char *GetProjectPath()							{ return m_ProjectPath.Get(); }
	void SetProjectPath( const char *pProjectPath )			{ m_ProjectPath = pProjectPath; }

    const char *GetSourceFileConfigFilter()                 { return m_sourceFileConfigFilter.Get(); }
    bool IsConfigAllowedBySourceFileConfigFilter( const char *pConfigName ) const
    {
        return m_sourceFileConfigFilter.IsEmpty() || !V_stricmp_fast( pConfigName, m_sourceFileConfigFilter );
    }
    
    CUtlStringBuilder* GetTempStringBuffer1()               { return &m_TempStringBuffer1; }
    CUtlStringBuilder* GetTempStringBuffer2()               { return &m_TempStringBuffer2; }
    CUtlStringBuilder* GetMacroReplaceBuffer()              { return &m_MacroReplaceBuffer; }
    CUtlStringBuilder* GetPropertyValueBuffer()             { return &m_PropertyValueBuffer; }
    
    const char *FormatTemp1( const char *pFormat, ... )
    {
        va_list args;
        va_start( args, pFormat );
        m_TempStringBuffer1.VFormat( pFormat, args );
        va_end( args );
        return m_TempStringBuffer1.Get();
    }

    const char *CreateGeneratedRootFilePath( CUtlPathStringHolder *pBuf, const char *pFile, const char *pSuffix = nullptr);
    const char *CreateGeneratedSubdirPath( CUtlPathStringHolder *pBuf, const char *pTopLevelName );
    
	int			ProcessCommandLine();


	IBaseProjectGenerator	*GetProjectGenerator() const { return m_pProjectGenerator; }
	void					SetProjectGenerator( IBaseProjectGenerator *pGenerator )	{ m_pProjectGenerator = pGenerator; }

	IBaseSolutionGenerator	*GetSolutionGenerator() const { return m_pSolutionGenerator; }

	// Iterates all the projects in the specified list, checks their conditionals, and calls pIterator->VisitProject for
	// each one that passes the conditional tests.
	//
	// If bForce is false, then it does a CRC check before visiting any project to see if the target project file is
	// already up-to-date with its .vpc file.
	void					IterateTargetProjects( CUtlVector<projectIndex_t> &projectList, IProjectIterator *pIterator );

	void					BuildTargetProjectScript(IProjectIterator* pIterator, int projectIdx, script_t* pProjectScript);

	bool					ParseProjectScript( const char *pScriptName, int depth, bool bQuiet, bool bWriteCRCCheckFile, CDependency_Project *pDependencyProject = nullptr );

	void					AddScriptToParsedList( const char *pScriptName, bool bAddToCRCCheck, CRC32_t crc = 0 );

	const char				*KeywordToName( configKeyword_e keyword );
	configKeyword_e			NameToKeyword( const char *pKeywordName );

	void 					SetupAllGames( bool bSet );

	int						GetProjectsInGroup( CUtlVector< projectIndex_t > &projectList, const char *pGroupHame );

	void					CreateVSAddinMetadataString( void );

	void					DetermineProjectGenerator();

	bool					IsTestMode( void ) const { return m_bTestMode; }
	bool					OutputName_ShouldAppendSrvToDedicated( void ) const { return m_bAppendSrvToDedicated; }
	bool					OutputName_ShouldAddUnitySuffix( void ) const { return m_bAddUnitySuffix; }
	const CUtlString &		OutputName_ProjectSuffixString( void ) const { return m_ProjectSuffixString; }

	const project_t *		GetProjectFromIndex( projectIndex_t nIndex ) const { return &m_Projects[nIndex]; }

	bool					BuildDependencyProjects( CUtlVector< CDependency_Project *> &projects );

	// TODO: should actually be private, but used in CConditionalStorage
	void					SetSystemConditional(char const* name, bool value);

private:
	void					SpewUsage( void );

	void					InProcessCRCCheck() const;

	void					DetermineSourcePath();
	void					SetDefaultSourcePath();

	void					DetermineSolutionGenerator();
	void					SetMacrosAndConditionals();

	void					SetVerbosityFromCommandLineArgs();
	void					HandleSingleCommandLineArg( const char *pArg );
	void					ParseBuildOptions(int argc, char const* const* argv);

	bool					CheckBinPath( char *pOutBinPath, int outBinPathSize );
	bool					RestartFromCorrectLocation( bool *pIsChild );

	void					GenerateOptionsCRCString();
	void					FindProjectFromVCPROJ(const char *pScriptNameVCProj, int nMainArgc, char const* const* pMainArgv);
	const char				*BuildTempGroupScript( const char *pScriptName );

	bool					AreSolutionDepenenciesActual(CUtlPathStringHolder dependenciesPath, const CUtlVector<CDependency_Project*>& referencedProjects);
	void					WriteSolutionDependencies(CUtlPathStringHolder dependenciesPath, const CUtlVector<CDependency_Project*>& referencedProjects);
	void					HandleMKSLN( IBaseSolutionGenerator *pSolutionGenerator,
	                                     IBaseSolutionGenerator *pSolutionGenerator2,
	                                     CProjectDependencyGraph &dependencyGraph );

	void					GenerateBuildSet( CProjectDependencyGraph &dependencyGraph );
	bool					BuildTargetProjects();
	bool					BuildTargetProject( IProjectIterator *pIterator, projectIndex_t projectIndex, script_t *pProjectScript, const char *pGameName );

	void					SaveConditionals();
	void					RestoreConditionals();

	bool					m_bQuietValidSpew;
	bool					m_bUsageOnly;
	bool					m_bHelp;
	bool					m_bSpewPlatforms;
	bool					m_bSpewGames;
	bool					m_bSpewGroups;
	bool					m_bSpewProjects;
	bool					m_bSpewProperties;
	bool					m_bTestMode;
	bool					m_bForceGenerate;
	bool					m_bForceIterate;
	bool					m_bEnableVpcGameMacro;
	bool    				m_bCheckFiles;
	bool					m_bDecorateProject;
	bool					m_bShowDeps;
	bool					m_bDedicatedBuild;
	bool					m_bAppendSrvToDedicated;	// concat "_srv" to dedicated server .so's.
	bool					m_bUseValveBinDir;			// On Linux, use gcc toolchain from /valve/bin/
	bool					m_bAnyProjectQualified;
	bool					m_bUse2010;
	bool					m_bPreferVS2010;
	bool					m_bUse2012;
	bool					m_bPreferVS2012;
	bool					m_bUse2013;
	bool					m_bPreferVS2013;
	bool					m_bUse2015;
	bool					m_bPreferVS2015;
	bool					m_bUse2022;
	bool					m_bPreferVS2022;
	bool					m_bSourceControl;
	bool					m_bAllowOSMacro;
	bool					m_bCRCCheckInProject;
	bool					m_bMissingFileIsError; 
	bool					m_bAllowFilePattern;
	bool					m_bAddExecuteableToCRC;
	bool					m_bRestrictProjects;
	bool					m_bForceRebuildCache;
	bool    				m_bShowFixedPaths;
	bool					m_bShowCaseIssues;
    bool                    m_bGenMakeProj;
    bool                    m_bPerFileCompileConfig;
    bool					m_bAllowLibWithinLib;

	bool					m_bAllowQt;
	bool					m_bAllowSchema;
	bool					m_bAllowUnity;
	bool					m_bAllowClang;
	bool					m_bEmitClangProject;

	// when set, .libs are not treated as dependencies, causing only direct source code dependencies to count
	bool					m_bShallowDepencies;

	// How many of the files listed in the VPC files are missing?
	int						m_nFilesMissing;
	int						m_nTotalFilesMissing;

	int						m_nArgc;
	char const* const*		m_ppArgv;



	// Path where vpc was started from
	CUtlString				m_StartDirectory;

	// Root path to the sources (i.e. the directory where the vpc_scripts directory can be found in).
	CUtlString				m_SourcePath;

	// path to the project being processed (i.e. the directory where this project's .vpc can be found).
	CUtlString				m_ProjectPath;

	// strings derived from command-line commands which is checked alongside project CRCs:
	CUtlString				m_SupplementalCRCString;

	CUtlSortVector< CUtlString, CUtlStringCaseLess >	m_ExtraOptionsForCRC;

	CUtlString				m_VSAddinMetadata;

	CUtlString				m_MKSolutionFilename;

	CUtlString				m_SolutionItemsFilename;	// For /slnitems

	CUtlString				m_ProjectName;
	CUtlString				m_LoadAddressName;
	CUtlString				m_GameName;

	CUtlString				m_ProjectSuffixString;
	CUtlString				m_OutputMirrorString;

	CUtlPathStringHolder	m_TempGroupScriptFilename;

    CUtlString              m_sourceFileConfigFilter;
    
	CUtlVector< CUtlString > m_SolutionFolderNames;	// For /slnfolders

	// This abstracts the differences between different output methods.
	IBaseProjectGenerator			*m_pProjectGenerator;
	IBaseSolutionGenerator			*m_pSolutionGenerator;
	IBaseSolutionGenerator			*m_pSolutionGenerator2;

	CUtlVector< CUtlString >		m_BuildCommands;

	CUtlVector< conditional_t* >	m_SavedConditionals;

    CUtlStringBuilder               m_TempStringBuffer1;
    CUtlStringBuilder               m_TempStringBuffer2;
    CUtlStringBuilder               m_MacroReplaceBuffer;
    CUtlStringBuilder               m_PropertyValueBuffer;

    
public:
	


	CUtlVector< scriptList_t >		m_ScriptList;

	CUtlVector< project_t >			m_Projects;
	CUtlVector< projectIndex_t >	m_TargetProjects;

	CUtlVector< group_t >			m_Groups;
	CUtlVector< groupTag_t >		m_GroupTags;

	// for script extensions
	struct CustomBuildStepForExtension_t
	{
		CUtlString m_BuildSteps;
		CUtlString m_DefinedInFile;
		int m_nDefinitionStartLine;
	};
	CUtlDict< CustomBuildStepForExtension_t, int > 	m_CustomBuildSteps;
	CUtlDict< CUtlString, int > 	m_CustomAutoScripts;

    CUtlDict< CCopyableUtlVector< CUtlString >, int >     m_LibraryDependencies;

    bool                            m_bInProjectSection;
	bool							m_bGeneratedProject;
	bool							m_bIsDependencyPass; // True inside CProjectDependencyGraph::BuildProjectDependencies()


	// Schema stuff
	CUtlVector< CUtlStringCI >		m_SchemaFiles;		// NOTE: case-insensitive comparisons (error-tolerant w.r.t filenames in .VPCs)
	CUtlStringMap< CUtlString >		m_SchemaOutputFileMap;

	// Qt stuff
	CUtlVector< CUtlStringCI >		m_QtFiles;
	CUtlStringMap< CUtlString >		m_QtOutputFileMap;

	// Unity file stuff
	CUtlDict< bool > 				m_UnityFilesSeen;
	CUtlStringMap< CUtlString >		m_UnityOutputFileMap;
	bool							m_bAddUnitySuffix;
	bool							m_bProjectUsesUnity;
	bool							m_bUnitySchemaHeadersOnly;
	// Should writable files (presumed to be checked out) be put in unity files? Needed for buildbot where all files are writable.
	bool							m_bUnityOnWritableFiles;
	bool							m_bDoneOnParseProjectEnd;


	CDependency_Project				*m_pDependencyProject;

public:
	CMacroStorage macros;
	CConditionalStorage conditionals;
	ProjectCache projectCache;
};

extern CVPC *g_pVPC;



// TODO: unify all generated files into one folder tree under '.\_VPC_' MOC (DONE: unity, clang)
extern const char			*g_VPCGeneratedFolderName;					
extern const char			*g_QtFolderName;							
extern const char			*g_SchemaFolderName;						
extern const char			*g_SchemaAnchorBase;						
extern const char			*g_IncludeSeparators[2];

extern void					VPC_GenerateProjectDependencies( CBaseProjectDataCollector *pDataCollector );
extern bool					VPC_AreProjectDependenciesSupportedForThisTargetPlatform( void );



//resolves common MSVC properties found in a string $(IntDir), $(TargetFile), ...
// returns the input string if no replacements are made, returns outputScratchSpace.Get() if replacements are performed.
extern const char *			VPC_ResolveCompilerMacrosInString( const char *szSourceString, CUtlString &outputScratchSpace, CProjectConfiguration *pRootConfig, CProjectConfiguration *pFileConfig );

// Get a list of preprocessor defines or include directories
// [NOTE: quotes are stripped before returning - the caller may need to add quotes depending on usage]
extern void					VPC_GetPreprocessorDefines( CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlVector< CUtlString > &defines );
extern void					VPC_GetIncludeDirectories(  CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlVector< CUtlString > &includes );


// ---------------- Qt feature --------------------------
extern void					VPC_Qt_OnParseProjectStart( void );
extern void					VPC_Qt_OnParseProjectEnd( class CVCProjGenerator *pDataCollector );
extern void					VPC_Qt_TrackFile( const char *pName, bool bRemove, VpcFileFlags_t iFileFlags );
extern CProjectFile *		VPC_Qt_GetGeneratedFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
// ------------------------------------------------------


// ---------------- Schema feature ----------------------
extern void					VPC_Schema_OnParseProjectStart( void );
extern void					VPC_Schema_OnParseProjectEnd( CVCProjGenerator *pDataCollector );
extern void					VPC_Schema_TrackFile( const char *pName, bool bRemove, VpcFileFlags_t iFileFlags );
extern void					VPC_Schema_ForceAdditionalDependencies( const char *pProjectName );
extern CProjectFile *		VPC_Schema_GetGeneratedFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
// ------------------------------------------------------


// ---------------- Unity files feature -----------------
extern void					VPC_Unity_OnParseProjectStart( void );
extern void					VPC_Unity_OnParseProjectEnd( CVCProjGenerator *pDataCollector );
extern bool					VPC_Unity_UpdateUnityFiles(char const* const* ppArgs, int nArgs);
extern CProjectFile *		VPC_Unity_GetContainingUnityFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
// ------------------------------------------------------


// Get the included PCH file (returns "" if none)
//  - sets 'bCreatesPCH'  to true if this file *creates* the PCH file
//  - sets 'bExcludesPCH' to true if this file is specifically configured to *not* use a PCH file
extern void					VPC_GetPCHInclude( CProjectFile *pFile, CProjectConfiguration *pRootConfig, CUtlString &pchFile, bool &bCreatesPCH, bool &bExcludesPCH );
// Get info describing all PCH files created by a project
//  - pchIncludeNames:	  returns the list of PCH header files (e.g cbase.h)
//    pchCreatorNames:    returns the list of files used to *create* the PCHs (e.g stdafx.cpp)
//    [NOTE: these two lists are parallel; always the same length, with no empty string entries]
//  - pRequiredPCHs:      (optional) can be used to filter the returned lists down to a subset of required PCHs
//  - pFilesExcludingPCH: (optional) receives a list of all files specifically configured to NOT use PCHs
extern void					VPC_GeneratePCHInfo(	CVCProjGenerator *pDataCollector, CProjectConfiguration *pRootConfig,
													CUtlVector< CUtlString > &pchIncludeNames, CUtlVector< CUtlString > &pchCreatorNames,
													CUtlVector< CUtlString > const *pRequiredPCHs = nullptr,
													CUtlVector< CProjectFile * > *pFilesExcludingPCH = nullptr);

// ---------------- Clang feature -----------------------
extern void					VPC_Clang_OnParseProjectEnd( CVCProjGenerator *pDataCollector );
// ------------------------------------------------------


// -------------- Build-generated files -----------------
struct CSourceFileInfo
{
	CSourceFileInfo( int folderIndex = -1 );
	CProjectFile *	m_pSourceFile;			// The input source file
	CProjectFile *	m_pDebugCompiledFile;	// The output file, compiled for debug   (may be different to m_pSourceFile for schema/qt/protobuf files)
	CProjectFile *	m_pReleaseCompiledFile;	// The output file, compiled for release (may be different to m_pSourceFile for schema/qt/protobuf files)
	CProjectFile *	m_pContainingUnityFile;	// The containing unity file for the output file(s), if any
	CUtlString		m_ConfigString;			// Constructed from the configuration properties of the output files
	CRC32_t			m_ConfigStringCRC;		// CRC of m_ConfigString, used for sorting in CUnityInputFileInfoLessFunc
	CUtlString		m_PCHName;				// The PCH file (if any) used by the input file
	bool			m_bCreatesPCH;			// Whether this file is used to create a PCH file
	int				m_iFolderIndex;			// Index of the input file in its folder, used as a tie-breaker during sorting
};

// Generates a CSourceFileInfo for a given source file
//  - return false on error, or for non-source files (dynamic files or non-compiled files)
extern bool					VPC_GeneratedFiles_GetSourceFileInfo(	CSourceFileInfo &result, CProjectFile *pFile, bool bGenerateConfigString,
																	CVCProjGenerator *pDataCollector, const CUtlVector< CProjectConfiguration * > &rootConfigs );
extern void					VPC_GeneratedFiles_OnParseProjectEnd(	CVCProjGenerator *pDataCollector );
// ------------------------------------------------------
