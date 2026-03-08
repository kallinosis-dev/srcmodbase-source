//====== Copyright 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#pragma once
#include "vpc.h"

class IBaseProjectGenerator;

class CProjectDependencyGraph;
enum k_EDependsOnFlags
{ 
	k_EDependsOnFlagCheckNormalDependencies		= 0x01,
	k_EDependsOnFlagCheckAdditionalDependencies	= 0x02,
	k_EDependsOnFlagRecurse						= 0x04
};

// Flags to CProjectDependencyGraph::BuildProjectDependencies.
#define BUILDPROJDEPS_CHECK_ALL_PROJECTS		0x01		// If set, uses the set of allowed .vpc files, otherwise restricted to the projects specified on the CL.

class CDependency_Project;

class CDependency
{
friend class CProjectDependencyGraph;
friend class CSingleProjectScanner;

public:
	CDependency( CProjectDependencyGraph *pDependencyGraph );
	virtual ~CDependency();

	// Flags are a combination of k_EDependsOnFlags.
	bool DependsOn( CDependency *pTest, int flags=k_EDependsOnFlagCheckNormalDependencies | k_EDependsOnFlagRecurse );
	const char* GetName() const;

	// Returns true if the absolute filename of this thing (CDependency::m_Filename) matches the absolute path specified.
	bool CompareAbsoluteFilename( const char *pAbsPath ) const;

	// Returns true if any direct dependencies of are found:
	bool GetDirectDependencies( CUtlVector< CDependency * > &result ) const;

	virtual CDependency_Project* GetDependencyProject() { return nullptr; }
	virtual CDependency_Project const* GetDependencyProject() const { return nullptr; }


	// This is full path to the VPC filename for a project (use CDependency_Project::GetProjectFileName() for the VCPROJ/VPJ filename).
	CUtlString m_Filename;

	// Files that this depends on.
	CUtlVector<CDependency*> m_Dependencies;

	// Additional Files provided by $AdditionalProjectDependencies. This is in a separate list because we don't
	// always want DependsOn() to check this.
	CUtlVector<CDependency*> m_AdditionalDependencies;

private:
	bool FindDependency_Internal( CUtlVector<CUtlBuffer> &callTreeOutputStack, CDependency *pTest, int flags, int depth );
	void Mark();
	bool HasBeenMarked() const;

	unsigned int m_iDependencyMark;

protected:
	CProjectDependencyGraph *m_pDependencyGraph;
};


// This represents a project (.vcproj) file, NOT a project like a projectIndex_t.
// There can be separate .vcproj files (and thus separate CDependency_Project) for each game and platform of a projectIndex_t.
class CDependency_Project : public CDependency
{
public:
	typedef CDependency BaseClass;

	CDependency_Project( CProjectDependencyGraph *pDependencyGraph );

	CDependency_Project* GetDependencyProject() override { return this; }
	CDependency_Project const* GetDependencyProject() const override { return this; }

public:
	// Straight out of the $AdditionalProjectDependencies key (split on semicolons).
	CUtlVector<CUtlString> m_AdditionalProjectDependencies;

	CUtlString	m_ProjectName;		// This comes from the $Project key in the .vpc file.

	const char *GetProjectFileName( void );
	const char *GetProjectGUIDString( void );

	// Note that there can be multiple CDependency_Projects with the same m_iProjectIndex.
	projectIndex_t m_iProjectIndex;

	IBaseProjectGenerator *m_pProjectGenerator;
};


// This class builds a graph of all dependencies, starting at the projects.
class CProjectDependencyGraph : public IProjectIterator
{
	friend class CDependency;
	friend class CDependency_Project;

public:
	CProjectDependencyGraph();

	// This is the main function to generate dependencies.
	// nBuildProjectDepsFlags is a combination of BUILDPROJDEPS_ flags.
	void BuildProjectDependencies( int nBuildProjectDepsFlags, CUtlVector< projectIndex_t > *pAllowedProjects = nullptr, CUtlVector< projectIndex_t > *pOverrideProjects = nullptr);

	bool HasGeneratedDependencies() const;

	CDependency* FindDependency( const char *pFilename, CUtlPathStringHolder *pFixedFilename = nullptr);
	CDependency* FindOrCreateDependency( const char *pFilename );

	// Look for all projects (that we've scanned during BuildProjectDependencies) that depend on the specified project.
	// If bDownwards is true,  then it adds iProject and all projects that _it depends on_.
	// If bDownwards is false, then it adds iProject and all projects that _depend on it_.
	void GetProjectDependencyTree( projectIndex_t iProject, CUtlVector<projectIndex_t> &dependentProjects, bool bDownwards );

	// This solves the central mismatch between the way VPC references projects and the way the CDependency stuff does.
	//
	// - VPC uses projectIndex_t, but a single projectIndex_t can turn into multiple games (server_tf, server_episodic, etc) in VPC_IterateTargetProjects.
	// - The dependency code has a separate CDependency_Project for each game.
	// 
	// This takes a bunch of project indices (usually m_targetProjects, which comes from the command line's "+this -that *theother" syntax), 
	// which are game-agnostic, and based on what games were specified on the command line, it builds the list of CDependency_Project*s.
	void TranslateProjectIndicesToDependencyProjects( CUtlVector<projectIndex_t> &projectList, CUtlVector<CDependency_Project*> &out ) const;

	// Use ClearAllDependencyMarks with CDependency::HasBeenMarked/Mark() to optimize graph traversal
	void ClearAllDependencyMarks();

// IProjectIterator overrides.
protected:
bool VisitProject( projectIndex_t iProject, const char *szProjectName ) override;

private:
	void ResolveAdditionalProjectDependencies();

public:
	// Projects and everything they depend on.
	CUtlVector<CDependency_Project*> m_Projects;
	CUtlDict<CDependency*,int> m_AllFiles;	// All files go in here. They should never be duplicated. These are indexed by the full filename (except .lib files, which have that stripped off).

private:
	CDebugContext _debugCtx;

	// Used when sweeping the dependency graph to prevent looping around forever.
	unsigned int		m_iDependencyMark;
	bool m_bHasGeneratedDependencies;	// Set to true after finishing BuildProjectDependencies.
};


bool IsLibraryFile( const char *pFilename );
