//========= Copyright Valve Corporation, All rights reserved. =========================//
//
// Purpose: Schema Script Generation
//
//=====================================================================================//

#pragma once
#include "projectscript.h"

class CProjectFile;
class CVCProjGenerator;


extern void					VPC_Schema_OnParseProjectStart( void );
extern void					VPC_Schema_OnParseProjectEnd( CVCProjGenerator *pDataCollector );
extern void					VPC_Schema_TrackFile( const char *pName, bool bRemove, VpcFileFlags_t iFileFlags );
extern void					VPC_Schema_ForceAdditionalDependencies( const char *pProjectName );
extern CProjectFile *		VPC_Schema_GetGeneratedFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
