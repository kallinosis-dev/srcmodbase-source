//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "materialsystem_global.h"
#include "shaderapi/ishaderapi.h"
#include "shadersystem.h"
#ifndef _PS3
#include <malloc.h>
#endif
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_FrameNum;

#ifndef SHADERAPI_GLOBAL_H
IShaderAPI *g_pShaderAPI = nullptr;
IShaderDeviceMgr* g_pShaderDeviceMgr = nullptr;
IShaderDevice *g_pShaderDevice = nullptr;
IShaderShadow* g_pShaderShadow = nullptr;
#endif

IClientMaterialSystem *g_pClientMaterialSystem = nullptr;