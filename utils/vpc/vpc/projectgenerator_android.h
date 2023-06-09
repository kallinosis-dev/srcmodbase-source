//============= Copyright Valve Corporation, All rights reserved. =============
//
// Purpose: 
//
//=============================================================================

#ifndef PROJECTGENERATOR_ANDROID_H
#define PROJECTGENERATOR_ANDROID_H

#pragma once

#include "sys_utils.h"
#include "ibaseprojectgenerator.h"
#include "projectgenerator_vcproj.h"

#define PROPERTYNAME( X, Y ) X##_##Y,
enum AndroidProperties_e
{
	#include "projectgenerator_android.inc"
};

class CProjectGenerator_Android : public CBaseProjectDataCollector
{
public:
	typedef CBaseProjectDataCollector BaseClass;
	CProjectGenerator_Android();

	const char* GetProjectFileExtension() override { return "androidproj"; }
	void EndProject( bool bSaveData ) override;

	CGeneratorDefinition	m_GeneratorDefinition;

	const char *GetTargetAndroidPlatformName( const char *szVPCPlatformName );

	bool WriteAndroidProj( CSpecificConfig *pBaseConfig, const CUtlVector<CSpecificConfig *> &generalConfigurations );
	bool WriteBuildXML( CSpecificConfig *pBaseConfig, const CUtlVector<CSpecificConfig *> &generalConfigurations );
	bool WriteAndroidManifestXML( CSpecificConfig *pBaseConfig, const CUtlVector<CSpecificConfig *> &generalConfigurations );
	bool WriteProjectProperties( CSpecificConfig *pBaseConfig, const CUtlVector<CSpecificConfig *> &generalConfigurations );

	void EnumerateSupportedVPCTargetPlatforms( CUtlVector<CUtlString> &output ) override;
	bool BuildsForTargetPlatform( const char *szVPCTargetPlatform ) override;
	bool DeploysForVPCTargetPlatform( const char *szVPCTargetPlatform ) override;
	CUtlString GetSolutionPlatformAlias( const char *szVPCTargetPlatform, IBaseSolutionGenerator *pSolutionGenerator ) override;
};

#endif // PROJECTGENERATOR_ANDROID_H
