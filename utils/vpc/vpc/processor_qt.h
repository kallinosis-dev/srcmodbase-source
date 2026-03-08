//========= Copyright Valve Corporation, All rights reserved. =========================//
//
// Purpose: Qt MOC integration
//
//=====================================================================================//

#pragma once
#include "projectscript.h"

class CProjectFile;


extern void					VPC_Qt_OnParseProjectStart();
extern void					VPC_Qt_OnParseProjectEnd( class CVCProjGenerator *pDataCollector );
extern void					VPC_Qt_TrackFile( const char *pName, bool bRemove, VpcFileFlags_t iFileFlags );
extern CProjectFile *		VPC_Qt_GetGeneratedFile( CProjectFile *pInputFile, const char *pConfigName, CVCProjGenerator *pDataCollector );
