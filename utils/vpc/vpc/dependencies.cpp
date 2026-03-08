//======= Copyright 1996-2016, Valve Corporation, All rights reserved. ========
//
// Purpose: 
//
//=============================================================================

#include "dependencies.h"


#include "tier0/fasttimer.h"
#include "tier1/splitstring.h"
#include "tier1/utlvector.h"

#include "vpc.h"
#include "baseprojectgenerator.h"
#include "environment_utils.h"
#include "misc.h"
#include "projectgenerator_vcproj.h"
#include "scriptutil.h"

// ------------------------------------------------------------------------------------------------------- //
// CDependency functions.
// ------------------------------------------------------------------------------------------------------- //

CDependency::CDependency( CProjectDependencyGraph *pDependencyGraph ) :
	m_pDependencyGraph( pDependencyGraph )
{
	// ensure this dependency marker is initialized unmarked by ensuring inequality
	m_iDependencyMark = m_pDependencyGraph->m_iDependencyMark - 1;
}

CDependency::~CDependency()
{
}

const char* CDependency::GetName() const
{
	return m_Filename.String();
}

bool CDependency::CompareAbsoluteFilename( const char *pAbsPath  ) const
{
	return ( V_stricmp_fast( m_Filename.String(), pAbsPath ) == 0 );
}

bool CDependency::DependsOn( CDependency *pTest, int flags )
{
	m_pDependencyGraph->ClearAllDependencyMarks();
	CUtlVector<CUtlBuffer> callTreeOutputStack;
	if ( FindDependency_Internal( callTreeOutputStack, pTest, flags, 1 ) )
	{
		if ( g_pVPC->IsShowDependencies() )
		{
			Log_Msg( LOG_VPC, "-------------------------------------------------------------------------------\n" );
			Log_Msg( LOG_VPC, "%s\n", GetName() );
			for( int i = callTreeOutputStack.Count() - 1; i >= 0; i-- )
			{
				Log_Msg( LOG_VPC, "%s", ( const char * )callTreeOutputStack[i].Base() );
			}
			Log_Msg( LOG_VPC, "-------------------------------------------------------------------------------\n" );
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CDependency::FindDependency_Internal( CUtlVector<CUtlBuffer> &callTreeOutputStack, CDependency *pTest, int flags, int depth )
{
	if ( pTest == this )
		return true;
	
	// Don't revisit us.
	if ( HasBeenMarked() )
		return false;

	Mark();

	// Don't recurse further?
	if ( depth > 1 && !(flags & k_EDependsOnFlagRecurse) )
		return false;

	// Go through everything I depend on. If any of those things 
	for ( int iDepList=0; iDepList < 2; iDepList++ )
	{
		if ( iDepList == 1 && !(flags & k_EDependsOnFlagCheckAdditionalDependencies) )
			continue;

		CUtlVector<CDependency*> &depList = (iDepList == 0 ? m_Dependencies : m_AdditionalDependencies);

		for ( int i=0; i < depList.Count(); i++ )
		{
			CDependency *pChild = depList[i];
			if ( pChild->FindDependency_Internal( callTreeOutputStack, pTest, flags, depth+1 ) )
			{
				if ( g_pVPC->IsShowDependencies() )
				{
					CUtlString dependsString;
					dependsString.Format( "depends on %s\n", pChild->GetName() );

					int n = callTreeOutputStack.AddToTail();
					CUtlBuffer &b = callTreeOutputStack[n];

					b.EnsureCapacity( dependsString.Length() + 2 );
					b.PutString( dependsString.Get() );
					b.PutChar( 0 );
				}
				return true;
			}
		}
	}

	return false;
}

bool CDependency::GetDirectDependencies( CUtlVector< CDependency * > &result ) const
{
	for ( int iDepList = 0; iDepList < 2; iDepList++ )
	{
		const CUtlVector<CDependency*> &depList = ( iDepList == 0 ? m_Dependencies : m_AdditionalDependencies );
		for ( int i = 0; i < depList.Count(); i++ )
		{
			CDependency *pChild = depList[i];

			AssertDbg(!result.HasElement( pChild )); // Should be no dupes (slow check for large sets)
			result.AddToTail( pChild );
		}
	}
	return !!result.Count();
}

void CDependency::Mark()
{
	m_iDependencyMark = m_pDependencyGraph->m_iDependencyMark;
}

bool CDependency::HasBeenMarked() const
{
	return m_iDependencyMark == m_pDependencyGraph->m_iDependencyMark;
}


CDependency_Project::CDependency_Project( CProjectDependencyGraph *pDependencyGraph )
 :	CDependency( pDependencyGraph )
 , m_pProjectGenerator( nullptr )
{
}

const char *CDependency_Project::GetProjectFileName( void )
{
	MAKE_CONTEXTUAL_LOGGER(&m_pDependencyGraph->_debugCtx);

	if ( !m_pProjectGenerator )
	{
		log.Error( "Could not determine project file name for \"%s\"", m_Filename.Get() );
	}

	return m_pProjectGenerator->GetOutputFileName();
}

const char *CDependency_Project::GetProjectGUIDString( void )
{
	MAKE_CONTEXTUAL_LOGGER(&m_pDependencyGraph->_debugCtx);

	if ( !m_pProjectGenerator )
	{
		log.Error( "Could not determine project GUID for \"%s\"", m_Filename.Get() );
	}

	return m_pProjectGenerator->GetGUIDString();
}


// This is responsible for scanning a project file and pulling out:
// - a list of libraries it uses
// - the $AdditionalIncludeDirectories paths
// - a list of source files it uses
// - the name of the file it generates
class CSingleProjectScanner
{
public:
	CSingleProjectScanner()
	 :	m_pDependencyGraph(nullptr),
		m_pDependencyProject(nullptr),
		m_pDataCollector(nullptr),
		m_nDupeChecks( 0 )
	{
		Assert( !s_pSingleton );
		s_pSingleton = this;
	}

	~CSingleProjectScanner()
	{
		Assert( s_pSingleton == this );
		s_pSingleton = nullptr;
	}
	
	void ScanProjectFile( CProjectDependencyGraph *pGraph, const char *szScriptName, CDependency_Project *pProject )
	{
		m_ScriptName		 = szScriptName;
		m_pDependencyGraph	 = pGraph;
		m_pDependencyProject = pProject;

		// This has VPC parse the script and CVCProjGenerator collects all the data into lists of the
		// stuff we care about like source files and include paths. It will call back into OnEndProject,
		// from CVCProjGenerator::EndProject(), via VPC_GenerateProjectDependencies()
		g_pVPC->ParseProjectScript( szScriptName, 0, true, false, pProject );
	}

	void OnEndProject( CBaseProjectGenerator *pDataCollector )
	{
		m_pDataCollector = pDataCollector;
		m_pVCProjGenerator = dynamic_cast<CVCProjGenerator *>(pDataCollector);
		m_ProjectName    = pDataCollector->GetProjectName();
		g_pVPC->UndecorateProjectName( m_ProjectName );

		// Clear the dependency marks, which we will use to avoid multiply-processing files in SetupFilesList()
		m_pDependencyGraph->ClearAllDependencyMarks();

		if ( m_pVCProjGenerator )
		{
			CUtlVector<CProjectConfiguration *> rootConfigs;
			m_pVCProjGenerator->GetAllRootConfigurations( rootConfigs );
			for ( int i = 0; i < rootConfigs.Count(); i++ )
			{
				// TODO: SetupFilesList() will early-out the second time through, so any additional search paths in the Release
				//       config will be ignored! We should combine the two sets of search paths and call SetupFilesList() ONCE
				//       (it will warn if any ambiguities are found - which we should definitely avoid)
				SetupFilesList( m_pVCProjGenerator->GetRootFolder() );
				SetupAdditionalProjectDependencies( rootConfigs[i] );
				SetupBuildToolDependencies( rootConfigs[i] );
				SetupProjectOutputs( rootConfigs[i] );
			}
		}
		else
		{
			for ( int nFileIter = pDataCollector->m_Files.First(); pDataCollector->m_Files.IsValidIndex( nFileIter ); nFileIter = pDataCollector->m_Files.Next( nFileIter ) )
			{
				CFileConfig *pFile = pDataCollector->m_Files.Element( nFileIter );
				char sAbsolutePath[MAX_FIXED_PATH];
				V_MakeAbsolutePath( sAbsolutePath, sizeof( sAbsolutePath ), pFile->m_Filename.Get(), nullptr, k_bVPCForceLowerCase );

				// Only consider library references
				if ( (pFile->m_iFlags & (VPC_FILE_FLAGS_STATIC_LIB | VPC_FILE_FLAGS_IMPORT_LIB | VPC_FILE_FLAGS_SHARED_LIB)) == 0)
					continue;

				// Add an entry to the project for this file (but only once - not twice for debug+release!)
				CDependency *pDep = m_pDependencyGraph->FindOrCreateDependency( sAbsolutePath );

				if ( !pDep->HasBeenMarked() && pDep != m_pDependencyProject )
				{
					AssertDbg( !m_pDependencyProject->m_Dependencies.HasElement( pDep ) ); // HasBeenMarked() should prevent this (slow check for large sets)
					m_pDependencyProject->m_Dependencies.AddToTail( pDep );
					pDep->Mark();
				}
			}
		}
	}

	void SetupFilesList( CProjectFolder *pFolder )
	{
		for ( int iIndex = pFolder->m_Files.Head(); iIndex != pFolder->m_Files.InvalidIndex(); iIndex = pFolder->m_Files.Next( iIndex ) )
		{

			CProjectFile *pFile = pFolder->m_Files[iIndex];
			// Only consider library references
			if ( (pFile->m_iFlags & (VPC_FILE_FLAGS_STATIC_LIB | VPC_FILE_FLAGS_IMPORT_LIB)) == 0 )
				continue;

			// Don't bother with dynamic files; prefer keeping the dependency behaviour simple and predictable
			// (the code which generates the dynamic files can inject additional dependencies explicitly)
			if ( pFile->m_iFlags & VPC_FILE_FLAGS_DYNAMIC )
				continue;



			// If this file is excluded from all configs, skip it.
			// NOTE: Schema files are always excluded from the build (see VPC_Schema_TrackFile), 
			//       but we do want proper dependencies for them, so ignore exclusion for them.
			if ( pFile->m_Configs.Count() && !( pFile->m_iFlags & VPC_FILE_FLAGS_SCHEMA ) )
			{
				bool bExcluded, bIncluded = false;
				for ( int iConfig = 0; iConfig < pFile->m_Configs.Count(); iConfig++ )
				{
					if ( !VPC_GetPropertyBool( KEYWORD_GENERAL, nullptr, pFile->m_Configs[ iConfig ], g_pOption_ExcludedFromBuild, &bExcluded ) 
						|| !bExcluded )
						bIncluded = true; // Marked as NOT excluded (or not marked at all) -> included
				}
				if ( !bIncluded )
					continue;
			}

			// Make this an absolute path.
			char sAbsolutePath[MAX_FIXED_PATH];
			V_MakeAbsolutePath( sAbsolutePath, sizeof( sAbsolutePath ), pFile->m_Name.Get(), nullptr, k_bVPCForceLowerCase );
			
			// Add an entry to the project for this file (but only once - not twice for debug+release!)
			CDependency *pDep = m_pDependencyGraph->FindOrCreateDependency( sAbsolutePath );
			if ( !pDep->HasBeenMarked() && pDep != m_pDependencyProject )
			{
				AssertDbg( !m_pDependencyProject->m_Dependencies.HasElement( pDep ) ); // HasBeenMarked() should prevent this (slow check for large sets)
				m_pDependencyProject->m_Dependencies.AddToTail( pDep );
				pDep->Mark();
			}
		}

		// Recurse into child folders:
		for ( int iIndex = pFolder->m_Folders.Head(); iIndex != pFolder->m_Folders.InvalidIndex(); iIndex = pFolder->m_Folders.Next( iIndex ) )
		{
			SetupFilesList( pFolder->m_Folders[iIndex] );
		}
	}

	void SetupProjectOutputs( CProjectConfiguration *pRootConfig )
	{
		CUtlString tempString;
		CUtlVector<CUtlString> outputsForThisConfig; //collecting locally so we can resolve compiler macros and deduplicate before adding to the member list

		//read $AdditionalOutputFiles property
		if ( VPC_GetPropertyString( KEYWORD_GENERAL, pRootConfig, nullptr, g_pOption_AdditionalOutputFiles, &tempString ) )
		{
			CSplitString outStrings( tempString.Get(), ";" );
			for ( int i=0; i < outStrings.Count(); i++ )
			{
				outputsForThisConfig.AddToTail( outStrings[i] );
			}
		}

		//linker outputs
		tempString.Clear();
		VPC_GetPropertyString( KEYWORD_LINKER, pRootConfig, nullptr, g_pOption_ImportLibrary, &tempString );
		if ( !tempString.IsEmpty() )
		{
			outputsForThisConfig.AddToTail( tempString );
		}
		tempString.Clear();
		VPC_GetPropertyString( KEYWORD_LIBRARIAN, pRootConfig, nullptr, g_pOption_OutputFile, &tempString );
		if ( !tempString.IsEmpty() )
		{
			outputsForThisConfig.AddToTail( tempString );
		}
		tempString.Clear();
		VPC_GetPropertyString( KEYWORD_LINKER, pRootConfig, nullptr, g_pOption_OutputFile, &tempString );
		if ( !tempString.IsEmpty() )
		{
			outputsForThisConfig.AddToTail( tempString );
		}

		tempString.Clear();
		VPC_GetPropertyString( KEYWORD_GENERAL, pRootConfig, nullptr, "$GameOutputFile", &tempString );
		if ( !tempString.IsEmpty() )
		{
			outputsForThisConfig.AddToTail( tempString );
		}
		
		for ( int nOutputIter = 0; nOutputIter < outputsForThisConfig.Count(); ++nOutputIter )
		{
			//Resolve compiler macros
			const char *szResolve = VPC_ResolveCompilerMacrosInString( outputsForThisConfig[nOutputIter], tempString, pRootConfig, nullptr );

			//add to the member list while deduplicating
			bool bDuplicate = false;
			for ( int nDupeIter = 0; nDupeIter < m_ProjectOutputs.Count(); ++nDupeIter )
			{
				if ( V_strcmp( szResolve, m_ProjectOutputs[nDupeIter] ) == 0 )
				{
					bDuplicate = true;
					break;
				}
			}
			if ( !bDuplicate )
			{
				m_ProjectOutputs.AddToTail( szResolve );
			}
		}
	}

	void SetupAdditionalProjectDependencies( CProjectConfiguration *pRootConfig ) const
	{
		CUtlString cfgString;
		if ( VPC_GetPropertyString( KEYWORD_GENERAL, pRootConfig, nullptr, g_pOption_AdditionalProjectDependencies, &cfgString ) )
		{
			CSplitString outStrings ( cfgString.Get(), ";" );
			for ( int i=0; i < outStrings.Count(); i++ )
			{
				CUtlString projectName( outStrings[i] );
				if ( !m_pDependencyProject->m_AdditionalProjectDependencies.IsValidIndex( m_pDependencyProject->m_AdditionalProjectDependencies.Find( projectName ) ) )
				{
					m_pDependencyProject->m_AdditionalProjectDependencies.AddToTail( projectName );
				}
			}
		}
	}

	void SetupBuildToolDependencies( CProjectConfiguration *pRootConfig ) const
	{
		CUtlVector< CUtlString > splitStrings;
		auto addFileDependencies = 
			[&] ( const char *szSemiColonDelimitedList ) -> void
			{
				if ( !szSemiColonDelimitedList || !szSemiColonDelimitedList[0] )
					return;

				splitStrings.RemoveAll();
				V_SplitString( szSemiColonDelimitedList, ";", splitStrings );

				for ( int nSplitIter = 0; nSplitIter < splitStrings.Count(); ++nSplitIter )
				{
					CUtlString const& filename = splitStrings[nSplitIter];

					if ( filename.IsEmpty() || !IsLibraryFile(filename.Get()) )
						continue;
				
					char szDependencyAbsolutePath[MAX_FIXED_PATH];
					V_MakeAbsolutePath( szDependencyAbsolutePath, sizeof( szDependencyAbsolutePath ), filename, g_pVPC->GetProjectPath(), k_bVPCForceLowerCase );

					CDependency *pOODependency = m_pDependencyGraph->FindOrCreateDependency( szDependencyAbsolutePath );
					if ( !m_pDependencyProject->m_Dependencies.IsValidIndex( m_pDependencyProject->m_Dependencies.Find( pOODependency ) ) )
					{
						m_pDependencyProject->m_Dependencies.AddToTail( pOODependency );
					}
				}
			};

		//Note because it threw me off. This will not contain vpc generated files from the schema step (possibly Qt as well?) during the solution build order dependency generation phase
		CUtlVector< CProjectFile * > allFiles;
		m_pVCProjGenerator->GetAllProjectFiles( allFiles );
		for ( int nFileIter = 0; nFileIter < allFiles.Count(); ++nFileIter )
		{
			CProjectFile *pProjectFile = allFiles[nFileIter];

			//additional dependencies
			if ( m_pVCProjGenerator->HasFilePropertyValue( pProjectFile, pRootConfig->m_Name.Get(), 
				KEYWORD_CUSTOMBUILDSTEP, g_pOption_AdditionalDependencies ) )
			{
				addFileDependencies( m_pVCProjGenerator->GetPropertyValueAsString( pProjectFile, pRootConfig->m_Name.Get(), 
					KEYWORD_CUSTOMBUILDSTEP, g_pOption_AdditionalDependencies ) );
			}

			//order only file dependencies
			if ( m_pVCProjGenerator->HasFilePropertyValue( pProjectFile, pRootConfig->m_Name.Get(), 
				KEYWORD_CUSTOMBUILDSTEP, g_pOption_OrderOnlyFileDependencies ) )
			{
				addFileDependencies( m_pVCProjGenerator->GetPropertyValueAsString( pProjectFile, pRootConfig->m_Name.Get(), 
					KEYWORD_CUSTOMBUILDSTEP, g_pOption_OrderOnlyFileDependencies ) );
			}

			//order only project dependencies
			if ( m_pVCProjGenerator->HasFilePropertyValue( pProjectFile, pRootConfig->m_Name.Get(), 
				KEYWORD_CUSTOMBUILDSTEP, g_pOption_OrderOnlyProjectDependencies ) )
			{
				const char *szProjectDependencies = m_pVCProjGenerator->GetPropertyValueAsString( pProjectFile, pRootConfig->m_Name.Get(), 
					KEYWORD_CUSTOMBUILDSTEP, g_pOption_OrderOnlyProjectDependencies );

				if ( szProjectDependencies && szProjectDependencies[0] )
				{
					splitStrings.RemoveAll();
					V_SplitString( szProjectDependencies, ";", splitStrings );

					for ( int nSplitIter = 0; nSplitIter < splitStrings.Count(); ++nSplitIter )
					{
						if ( splitStrings[nSplitIter].IsEmpty() )
							continue;

						if ( !m_pDependencyProject->m_AdditionalProjectDependencies.IsValidIndex( m_pDependencyProject->m_AdditionalProjectDependencies.AddToTail( splitStrings[nSplitIter] ) ) )
						{
							m_pDependencyProject->m_AdditionalProjectDependencies.AddToTail( splitStrings[nSplitIter] );
						}
					}
				}
			}
		}
	}

public:
	// Project include directories. These strings are deleted when the object goes away.
	CProjectDependencyGraph *m_pDependencyGraph;
	CDependency_Project *m_pDependencyProject;
	CBaseProjectGenerator *m_pDataCollector;
	CVCProjGenerator *m_pVCProjGenerator;
	CUtlVector<CUtlString> m_ProjectOutputs;
	CUtlString m_ScriptName;
	CUtlString m_ProjectName;
	int m_nDupeChecks;

	static CSingleProjectScanner *s_pSingleton;
};

CSingleProjectScanner *CSingleProjectScanner::s_pSingleton = nullptr;
void VPC_GenerateProjectDependencies( CBaseProjectGenerator *pDataCollector )
{
	// This hooks into CVCProjGenerator::EndProject for dependency-extraction 
	// (instantiating CSingleProjectScanner sets up CSingleProjectScanner::s_pSingleton)
	if ( !CSingleProjectScanner::s_pSingleton )
		return;
	CSingleProjectScanner::s_pSingleton->OnEndProject( pDataCollector );
}

CProjectDependencyGraph::CProjectDependencyGraph(): _debugCtx{"Project dependencies"}
{
	m_iDependencyMark = 1;
	m_bHasGeneratedDependencies = false;
}

void CProjectDependencyGraph::BuildProjectDependencies( int nBuildProjectDepsFlags, CUtlVector< projectIndex_t > *pAllowedProjects, CUtlVector< projectIndex_t > *pOverrideProjects )
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	g_pVPC->m_bIsDependencyPass = true;

	// Have it iterate ALL projects in the list, with the current platform conditional.
	CUtlVector< projectIndex_t > projectList;
	CUtlVector< CUtlString > priorSetGames;

	// Build the list of projects to iterate:
	if ( pOverrideProjects )
	{
		// iterate just the given projects
		projectList.AddMultipleToTail( pOverrideProjects->Count(), pOverrideProjects->Base() );
		Assert( !( nBuildProjectDepsFlags & BUILDPROJDEPS_CHECK_ALL_PROJECTS ) ); // It's one or the other, bozo.
	}
	else if ( nBuildProjectDepsFlags & BUILDPROJDEPS_CHECK_ALL_PROJECTS )
	{
		// So iterate all projects.
		projectList.SetCount( g_pVPC->m_Projects.Count() );
		for ( int i=0; i < g_pVPC->m_Projects.Count(); i++ )
		{
			projectList[i] = i;
		}

		CUtlVector< projectIndex_t > everythingProjectsIndices;
		if ( g_pVPC->RestrictProjectsToEverything() )
		{
			if ( g_pVPC->GetProjectsInGroup( everythingProjectsIndices, "everything" ) )
			{
				CUtlVector< projectIndex_t > doomedProjectsIndices;
				for ( int i = 0; i < projectList.Count() ; i++ )
				{
					int nTargetProjectIndex = projectList[i];
					if ( everythingProjectsIndices.Find( nTargetProjectIndex ) == everythingProjectsIndices.InvalidIndex() )
					{
						if ( pAllowedProjects && pAllowedProjects->Find( nTargetProjectIndex ) != pAllowedProjects->InvalidIndex() )
						{
							// this project's consideration was overridden, it gets a pardon
							continue;
						}

						// the target project is not in the everything group so it gets removed
						doomedProjectsIndices.AddToTail( nTargetProjectIndex );
					}
				}

				for ( int i = 0; i < doomedProjectsIndices.Count(); i++ )
				{
					projectList.FindAndRemove( doomedProjectsIndices[i] );
				}
			}	
		}
	}
	else
	{
		// Iterate just the projects specified on the command-line
		projectList.AddMultipleToTail( g_pVPC->m_TargetProjects.Count(), g_pVPC->m_TargetProjects.Base() );
	}

	if ( projectList.Count() )
	{
		
		log.Status( "\nBuilding project dependency set (libs only)..." );

		if ( nBuildProjectDepsFlags & BUILDPROJDEPS_CHECK_ALL_PROJECTS )
		{
			// BUILDPROJDEPS_CHECK_ALL_PROJECTS forces all games which causes all the game based projects to be iterated
			// save current state of game defines
			for (conditional_t* game : g_pVPC->conditionals.GetAllDefined(CONDITIONAL_GAME))
				priorSetGames.AddToTail(game->m_Name.Get());

			// force all games
			g_pVPC->SetupAllGames( true );
		}

		// iterate projects, determine dependencies
		//CFastTimer timer;
		//timer.Start();
		logging::pacifier::Clear();
		g_pVPC->IterateTargetProjects( projectList, this );
		logging::pacifier::Break();
		//timer.End();

		// add in explicit dependencies
		ResolveAdditionalProjectDependencies();

		if ( nBuildProjectDepsFlags & BUILDPROJDEPS_CHECK_ALL_PROJECTS )
		{
			// Restore the old game defines state
			g_pVPC->SetupAllGames( false );
			for ( int j = 0; j < priorSetGames.Count(); j++ )
			{
				g_pVPC->conditionals.Set( priorSetGames[j].Get(), true, CONDITIONAL_GAME, nullptr );
			}
		}		
	}

	m_bHasGeneratedDependencies = true;

	g_pVPC->m_bIsDependencyPass = false;
}

void CProjectDependencyGraph::ResolveAdditionalProjectDependencies()
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	// projects support an explicit list of dependencies that need to be accounted for
	for ( int iMainProject=0; iMainProject < m_Projects.Count(); iMainProject++ )
	{
		CDependency_Project *pMainProject = m_Projects[iMainProject];

		// get the target project's dependency list
		for ( int i=0; i < pMainProject->m_AdditionalProjectDependencies.Count(); i++ )
		{
			const char *pLookingFor = pMainProject->m_AdditionalProjectDependencies[i].String();

			// Look for this project name among all the projects.
			int j;
			for ( j=0; j < m_Projects.Count(); j++ )
			{
				// TODO: this is broken for schemacompiler - need to port schemacompiler to win64
				//       (requires: ripping out Incredibuild integration and tweaking Buildbot)
				if ( V_stricmp_fast( m_Projects[j]->m_ProjectName.String(), pLookingFor ) == 0 )
				{
					// found
					if ( pMainProject->m_AdditionalDependencies.Find( m_Projects[j] ) == pMainProject->m_AdditionalDependencies.InvalidIndex() )
					{
						pMainProject->m_AdditionalDependencies.AddToTail( m_Projects[j] );
					}
					break;
				}
			}

			if ( logging::IsVerbose() && ( j == m_Projects.Count() ) )
			{
				// not found
				log.Warning( "Project '%s' lists '%s' in its $AdditionalProjectDependencies, but there is no project by that name.", pMainProject->GetName(), pLookingFor );
			}
		}
	}
}

bool CProjectDependencyGraph::HasGeneratedDependencies() const
{
	return m_bHasGeneratedDependencies;
}

bool CProjectDependencyGraph::VisitProject( projectIndex_t iProject, const char *szProjectName )
{
	MAKE_CONTEXTUAL_LOGGER_AUTO;

	if ( !VPC_AreProjectDependenciesSupportedForThisTargetPlatform() ) // Should error-out further upstream than here...
		log.Error( "Cannot build project dependencies, not supported for %s yet", g_pVPC->conditionals.GetTargetPlatformName() );

	// Read in the project.
	if ( !Sys_Exists( szProjectName ) )
	{
		return false;
	}

	logging::pacifier::Output();

	// Add this project.
	CDependency_Project *pProject = new CDependency_Project( this );
	
	char szAbsolute[MAX_FIXED_PATH];
	V_MakeAbsolutePath( szAbsolute, sizeof( szAbsolute ), szProjectName, nullptr, k_bVPCForceLowerCase );
	pProject->m_Filename = szAbsolute;

	pProject->m_iProjectIndex = iProject;
	m_Projects.AddToTail( pProject );
	m_AllFiles.Insert( szAbsolute, pProject );

	// Scan the project file and get all its libs, cpp, and h files.
	CSingleProjectScanner scanner;
	scanner.ScanProjectFile( this, szAbsolute, pProject );
	pProject->m_ProjectName = scanner.m_ProjectName;

	// Now add a CDependency for each output file.
	if ( !g_pVPC->UsingShallowDependencies() )
	{
		for ( int i = 0; i < scanner.m_ProjectOutputs.Count(); ++i )
		{
			const char *pFilename = scanner.m_ProjectOutputs[i].Get();

			if(!IsLibraryFile(pFilename))
				continue;

			// fixup the path and add it
			char szOutputAbsPath[MAX_FIXED_PATH];
			V_MakeAbsolutePath( szOutputAbsPath, sizeof( szOutputAbsPath ), pFilename, g_pVPC->GetProjectPath(), k_bVPCForceLowerCase );

			CDependency *pOutputDependency = FindOrCreateDependency( szOutputAbsPath );
			pOutputDependency->m_Dependencies.AddToTail( pProject );
			//Msg( " - ADDING DEPENDENCY: %s (%s)\n", szOutputAbsPath, pProject->m_ProjectName.Get() );
		}
	}

	return true;
}


void CProjectDependencyGraph::GetProjectDependencyTree( projectIndex_t iProject, CUtlVector<projectIndex_t> &dependentProjects, bool bDownwards )
{
	// add self
	if ( dependentProjects.Find( iProject ) == dependentProjects.InvalidIndex() )
	{
		dependentProjects.AddToTail( iProject );
	}

	// add anything that depends on it
	for ( int i=0; i < m_Projects.Count(); i++)
	{
		CDependency_Project *pProject = m_Projects[i];
		if ( pProject->m_iProjectIndex != iProject )
			continue;

		// found target project, find anything that depends on it
		for ( int iOther=0; iOther < m_Projects.Count(); iOther++ )
		{
			CDependency_Project *pOther = m_Projects[iOther];
			if ( pOther->m_iProjectIndex == iProject )
				continue;

			bool bThereIsADependency;
			if ( bDownwards )
			{
				bThereIsADependency = pProject->DependsOn( pOther, 
					k_EDependsOnFlagCheckNormalDependencies | k_EDependsOnFlagCheckAdditionalDependencies | k_EDependsOnFlagRecurse );
			}
			else
			{
				bThereIsADependency = pOther->DependsOn( pProject, 
					k_EDependsOnFlagCheckNormalDependencies | k_EDependsOnFlagCheckAdditionalDependencies | k_EDependsOnFlagRecurse );
			}

			if ( bThereIsADependency )
			{
				if ( dependentProjects.Find( pOther->m_iProjectIndex ) == dependentProjects.InvalidIndex() )
				{
					dependentProjects.AddToTail( pOther->m_iProjectIndex );
				}
			}
		}
	}
}

CDependency* CProjectDependencyGraph::FindDependency( const char *pFilename, CUtlPathStringHolder *pFixedFilename )
{
	// Normalize paths on entry to m_AllFiles; fix slashes, case and stuff like blah/../blah
	// (optionally returning the result to the caller, to avoid redundant work)
    CUtlPathStringHolder tmpFixupBuf;
    if ( pFixedFilename == nullptr)
    {
        pFixedFilename = &tmpFixupBuf;
    }
    pFixedFilename->Set( pFilename );
    pFixedFilename->FixupPathName();

	int i = m_AllFiles.Find( pFixedFilename->Get() );
	if ( i == m_AllFiles.InvalidIndex() )
		return nullptr;
	else
		return m_AllFiles[i];
}


CDependency* CProjectDependencyGraph::FindOrCreateDependency( const char *pFilename )
{
    CUtlPathStringHolder fixedFilename;
	CDependency *pDependency = FindDependency( pFilename, &fixedFilename );
	if ( pDependency )
		return pDependency;

    // Couldn't find it. Create one (using the fixed-up filename).
	pDependency = new CDependency( this );
	pDependency->m_Filename = fixedFilename;
	m_AllFiles.Insert( fixedFilename, pDependency );

	return pDependency;
}


void CProjectDependencyGraph::ClearAllDependencyMarks()
{
	if ( m_iDependencyMark == 0xFFFFFFFF )
	{
		// overflow, not expected to happen, rollover and reset
		m_iDependencyMark = 1;
		for ( int i=m_AllFiles.First(); i != m_AllFiles.InvalidIndex(); i=m_AllFiles.Next(i) )
		{
			m_AllFiles[i]->m_iDependencyMark = 0;
		}
	}
	else
	{
		// Advance the dependency marker, all other marks become unequal and therefore unmarked.
		++m_iDependencyMark;
	}
}

// This is called so we can translate from projectIndex_t to (CDependency_Project*)
class CProjectDependencyGraphProjectFilter : public IProjectIterator
{
public:
	explicit CProjectDependencyGraphProjectFilter(CDebugContext const* debugCtx): _debugCtx(debugCtx) {}

	bool VisitProject( projectIndex_t iProject, const char *szProjectName ) override
	{
		MAKE_CONTEXTUAL_LOGGER(_debugCtx);

		char szAbsolute[MAX_FIXED_PATH];
		V_MakeAbsolutePath( szAbsolute, sizeof( szAbsolute ), szProjectName, nullptr, k_bVPCForceLowerCase );
		
		// Ok, we've got an (absolute) project filename. Search the dependency graph for one with that name.
		bool bAdded = false;
		for ( int i=0; i < m_pAllProjectsList->Count(); i++ )
		{
			CDependency_Project *pProject = m_pAllProjectsList->Element( i );

			if ( pProject->CompareAbsoluteFilename( szAbsolute ) )
			{
				m_pOutProjectsList->AddToTail( pProject );
				bAdded = true;
				break;
			}
		}

		if ( !bAdded )
		{
			log.Warning( "Project Dependency Iteration: Project '%s' not recognized by dependency graph, skipping.\n"
				"Project is likely not part of \"Everything\" group.", szProjectName );
		}

		return true;
	}

public:
	CDebugContext const* _debugCtx;

	const CUtlVector<CDependency_Project*> *m_pAllProjectsList;
	CUtlVector<CDependency_Project*> *m_pOutProjectsList;
};

void CProjectDependencyGraph::TranslateProjectIndicesToDependencyProjects( CUtlVector<projectIndex_t> &projectList, CUtlVector<CDependency_Project*> &out ) const
{
	CProjectDependencyGraphProjectFilter iterator { &_debugCtx };
	iterator.m_pAllProjectsList = &m_Projects;
	iterator.m_pOutProjectsList = &out;

	g_pVPC->IterateTargetProjects( projectList, &iterator );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool VPC_AreProjectDependenciesSupportedForThisTargetPlatform( void )
{
	// Only supported for platforms that use CVCProjGenerator 
	// [ CDependencyGraph was switched to use CVCProjGenerator, due to bugs in CBaseProjectGenerator ]
	const char *pPlatformName = g_pVPC->conditionals.GetTargetPlatformName();

	bool bSupported = !V_stricmp_fast( pPlatformName, "WIN32" ) || 
					  !V_stricmp_fast( pPlatformName, "WIN64" )||
					  VPC_IsPlatformLinux( pPlatformName ) ||
					  VPC_IsPlatformAndroid( pPlatformName );
	return bSupported;
}
