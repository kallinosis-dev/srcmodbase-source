//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "interfaces/interfaces.h"


//-----------------------------------------------------------------------------
// Tier 1 libraries
//-----------------------------------------------------------------------------
ICvar *cvar = nullptr;
ICvar *g_pCVar = nullptr;
IProcessUtils *g_pProcessUtils = nullptr;
static bool s_bConnected = false;
IPhysics2 *g_pPhysics2 = nullptr;
IPhysics2ActorManager *g_pPhysics2ActorManager = nullptr;
IPhysics2ResourceManager *g_pPhysics2ResourceManager = nullptr;
IEventSystem *g_pEventSystem = nullptr;
ILocalize *g_pLocalize = nullptr;

// for utlsortvector.h
#ifndef _WIN32
void *g_pUtlSortVectorQSortContext = NULL;
#endif



//-----------------------------------------------------------------------------
// Tier 2 libraries
//-----------------------------------------------------------------------------
IResourceSystem *g_pResourceSystem = nullptr;
IRenderDeviceMgr *g_pRenderDeviceMgr = nullptr;
IFileSystem *g_pFullFileSystem = nullptr;
IAsyncFileSystem *g_pAsyncFileSystem = nullptr;
IMaterialSystem *materials = nullptr;
IMaterialSystem *g_pMaterialSystem = nullptr;
IMaterialSystem2 *g_pMaterialSystem2 = nullptr;
IInputSystem *g_pInputSystem = nullptr;
IInputStackSystem *g_pInputStackSystem = nullptr;
INetworkSystem *g_pNetworkSystem = nullptr;
ISoundSystem *g_pSoundSystem = nullptr;
IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig = nullptr;
IDebugTextureInfo *g_pMaterialSystemDebugTextureInfo = nullptr;
IVBAllocTracker *g_VBAllocTracker = nullptr;
IColorCorrectionSystem *colorcorrection = nullptr;
IP4 *p4 = nullptr;
IMdlLib *mdllib = nullptr;
IQueuedLoader *g_pQueuedLoader = nullptr;
IResourceAccessControl *g_pResourceAccessControl = nullptr;
IPrecacheSystem *g_pPrecacheSystem = nullptr;
ISceneSystem *g_pSceneSystem = nullptr;

#if defined( PLATFORM_X360 )
IXboxInstaller *g_pXboxInstaller = 0;
#endif

IMatchFramework *g_pMatchFramework = nullptr;
IGameUISystemMgr *g_pGameUISystemMgr = nullptr;

#if defined( INCLUDE_SCALEFORM )
IScaleformUI *g_pScaleformUI = 0;
#endif

//-----------------------------------------------------------------------------
// Not exactly a global, but we're going to keep track of these here anyways
//-----------------------------------------------------------------------------
IRenderDevice *g_pRenderDevice = nullptr;
IRenderHardwareConfig *g_pRenderHardwareConfig = nullptr;


//-----------------------------------------------------------------------------
// Tier3 libraries
//-----------------------------------------------------------------------------
IMeshSystem *g_pMeshSystem = nullptr;
IStudioRender *g_pStudioRender = nullptr;
IStudioRender *studiorender = nullptr;
IMatSystemSurface *g_pMatSystemSurface = nullptr;
vgui::IInput *g_pVGuiInput = nullptr;
vgui::ISurface *g_pVGuiSurface = nullptr;
vgui::IPanel *g_pVGuiPanel = nullptr;
vgui::IVGui	*g_pVGui = nullptr;
vgui::ILocalize *g_pVGuiLocalize = nullptr;
vgui::ISchemeManager *g_pVGuiSchemeManager = nullptr;
vgui::ISystem *g_pVGuiSystem = nullptr;
IDataCache *g_pDataCache = nullptr;
IMDLCache *g_pMDLCache = nullptr;
IMDLCache *mdlcache = nullptr;
IAvi *g_pAVI = nullptr;
IBik *g_pBIK = nullptr;
IDmeMakefileUtils *g_pDmeMakefileUtils = nullptr;
IPhysicsCollision *g_pPhysicsCollision = nullptr;
ISoundEmitterSystemBase *g_pSoundEmitterSystem = nullptr;
IWorldRendererMgr *g_pWorldRendererMgr = nullptr;
IVGuiRenderSurface *g_pVGuiRenderSurface = nullptr;


//-----------------------------------------------------------------------------
// Mapping of interface string to globals
//-----------------------------------------------------------------------------
struct InterfaceGlobals_t
{
	const char *m_pInterfaceName;
	void *m_ppGlobal;
};

static InterfaceGlobals_t g_pInterfaceGlobals[] =
{
	{ CVAR_INTERFACE_VERSION, &cvar },
	{ CVAR_INTERFACE_VERSION, &g_pCVar },
	{ EVENTSYSTEM_INTERFACE_VERSION, &g_pEventSystem },
	{ PROCESS_UTILS_INTERFACE_VERSION, &g_pProcessUtils },
	{ VPHYSICS2_INTERFACE_VERSION, &g_pPhysics2 },
	{ VPHYSICS2_ACTOR_MGR_INTERFACE_VERSION, &g_pPhysics2ActorManager },
	{ VPHYSICS2_RESOURCE_MGR_INTERFACE_VERSION, &g_pPhysics2ResourceManager },
	{ FILESYSTEM_INTERFACE_VERSION, &g_pFullFileSystem },
	{ ASYNCFILESYSTEM_INTERFACE_VERSION, &g_pAsyncFileSystem },
	{ RESOURCESYSTEM_INTERFACE_VERSION, &g_pResourceSystem },
	{ MATERIAL_SYSTEM_INTERFACE_VERSION, &g_pMaterialSystem },
	{ MATERIAL_SYSTEM_INTERFACE_VERSION, &materials },
	{ MATERIAL_SYSTEM2_INTERFACE_VERSION, &g_pMaterialSystem2 },
	{ INPUTSYSTEM_INTERFACE_VERSION, &g_pInputSystem },
	{ INPUTSTACKSYSTEM_INTERFACE_VERSION, &g_pInputStackSystem },
	{ NETWORKSYSTEM_INTERFACE_VERSION, &g_pNetworkSystem },
	{ RENDER_DEVICE_MGR_INTERFACE_VERSION, &g_pRenderDeviceMgr },
	{ MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, &g_pMaterialSystemHardwareConfig },
	{ SOUNDSYSTEM_INTERFACE_VERSION, &g_pSoundSystem },
	{ DEBUG_TEXTURE_INFO_VERSION, &g_pMaterialSystemDebugTextureInfo },
	{ VB_ALLOC_TRACKER_INTERFACE_VERSION, &g_VBAllocTracker },
	{ COLORCORRECTION_INTERFACE_VERSION, &colorcorrection },
	{ P4_INTERFACE_VERSION, &p4 },
	{ MDLLIB_INTERFACE_VERSION, &mdllib },
	{ QUEUEDLOADER_INTERFACE_VERSION, &g_pQueuedLoader },
	{ RESOURCE_ACCESS_CONTROL_INTERFACE_VERSION, &g_pResourceAccessControl },
	{ PRECACHE_SYSTEM_INTERFACE_VERSION, &g_pPrecacheSystem },
	{ STUDIO_RENDER_INTERFACE_VERSION, &g_pStudioRender },
	{ STUDIO_RENDER_INTERFACE_VERSION, &studiorender },
	{ VGUI_IVGUI_INTERFACE_VERSION, &g_pVGui },
	{ VGUI_INPUT_INTERFACE_VERSION, &g_pVGuiInput },
	{ VGUI_PANEL_INTERFACE_VERSION, &g_pVGuiPanel },
	{ VGUI_SURFACE_INTERFACE_VERSION, &g_pVGuiSurface },
	{ VGUI_SCHEME_INTERFACE_VERSION, &g_pVGuiSchemeManager },
	{ VGUI_SYSTEM_INTERFACE_VERSION, &g_pVGuiSystem },
	{ LOCALIZE_INTERFACE_VERSION, &g_pLocalize },
	{ LOCALIZE_INTERFACE_VERSION, &g_pVGuiLocalize },
	{ MAT_SYSTEM_SURFACE_INTERFACE_VERSION, &g_pMatSystemSurface },
	{ DATACACHE_INTERFACE_VERSION, &g_pDataCache },
	{ MDLCACHE_INTERFACE_VERSION, &g_pMDLCache },
	{ MDLCACHE_INTERFACE_VERSION, &mdlcache },
	{ AVI_INTERFACE_VERSION, &g_pAVI },
	{ BIK_INTERFACE_VERSION, &g_pBIK },
	{ DMEMAKEFILE_UTILS_INTERFACE_VERSION, &g_pDmeMakefileUtils },
	{ VPHYSICS_COLLISION_INTERFACE_VERSION, &g_pPhysicsCollision },
	{ SOUNDEMITTERSYSTEM_INTERFACE_VERSION, &g_pSoundEmitterSystem },
	{ MESHSYSTEM_INTERFACE_VERSION, &g_pMeshSystem },
	{ RENDER_DEVICE_INTERFACE_VERSION, &g_pRenderDevice },
	{ RENDER_HARDWARECONFIG_INTERFACE_VERSION, &g_pRenderHardwareConfig },
	{ SCENESYSTEM_INTERFACE_VERSION, &g_pSceneSystem },
	{ WORLD_RENDERER_MGR_INTERFACE_VERSION, &g_pWorldRendererMgr },
	{ RENDER_SYSTEM_SURFACE_INTERFACE_VERSION, &g_pVGuiRenderSurface },

#if defined( _X360 )
	{ XBOXINSTALLER_INTERFACE_VERSION, &g_pXboxInstaller },
#endif

	{ MATCHFRAMEWORK_INTERFACE_VERSION, &g_pMatchFramework },
	{ GAMEUISYSTEMMGR_INTERFACE_VERSION, &g_pGameUISystemMgr },

#if defined( INCLUDE_SCALEFORM )
	{ SCALEFORMUI_INTERFACE_VERSION, &g_pScaleformUI },
#endif

};


//-----------------------------------------------------------------------------
// The # of times this DLL has been connected
//-----------------------------------------------------------------------------
static int s_nConnectionCount = 0;


//-----------------------------------------------------------------------------
// At each level of connection, we're going to keep track of which interfaces
// we filled in. When we disconnect, we'll clear those interface pointers out.
//-----------------------------------------------------------------------------
struct ConnectionRegistration_t
{
	void *m_ppGlobalStorage;
	int m_nConnectionPhase;
};

static int s_nRegistrationCount = 0;
static ConnectionRegistration_t s_pConnectionRegistration[ ARRAYSIZE(g_pInterfaceGlobals) + 1 ];

void RegisterInterface( CreateInterfaceFn factory, const char *pInterfaceName, void **ppGlobal )
{
	if ( !(*ppGlobal) )
	{
		*ppGlobal = factory( pInterfaceName, nullptr);
		if ( *ppGlobal )
		{
			Assert( s_nRegistrationCount < ARRAYSIZE(s_pConnectionRegistration) );
			ConnectionRegistration_t &reg = s_pConnectionRegistration[s_nRegistrationCount++];
			reg.m_ppGlobalStorage = ppGlobal;
			reg.m_nConnectionPhase = s_nConnectionCount;
		}
	}
}

void ReconnectInterface( CreateInterfaceFn factory, const char *pInterfaceName, void **ppGlobal )
{
	*ppGlobal = factory( pInterfaceName, nullptr);

	bool bFound = false;
	Assert( s_nRegistrationCount < ARRAYSIZE(s_pConnectionRegistration) );
	for ( int i = 0; i < s_nRegistrationCount; ++i )
	{
		ConnectionRegistration_t &reg = s_pConnectionRegistration[i];
		if ( reg.m_ppGlobalStorage != ppGlobal )
			continue;

		reg.m_ppGlobalStorage = ppGlobal;
		bFound = true;
	}

	if ( !bFound && *ppGlobal )
	{
		Assert( s_nRegistrationCount < ARRAYSIZE(s_pConnectionRegistration) );
		ConnectionRegistration_t &reg = s_pConnectionRegistration[s_nRegistrationCount++];
		reg.m_ppGlobalStorage = ppGlobal;
		reg.m_nConnectionPhase = s_nConnectionCount;
	}
}


//-----------------------------------------------------------------------------
// Call this to connect to all tier 1 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectInterfaces( CreateInterfaceFn *pFactoryList, int nFactoryCount )
{
	if ( s_nRegistrationCount < 0 )
	{
		Error( "APPSYSTEM: In ConnectInterfaces(), s_nRegistrationCount is %d!\n", s_nRegistrationCount );
	}
	else if ( s_nRegistrationCount == 0 )
	{
		for ( int i = 0; i < nFactoryCount; ++i )
		{
			for ( int j = 0; j < ARRAYSIZE( g_pInterfaceGlobals ); ++j )
			{
				RegisterInterface( pFactoryList[i], g_pInterfaceGlobals[j].m_pInterfaceName, (void**)g_pInterfaceGlobals[j].m_ppGlobal );
			}
		}
	}
	else
	{
		// This is no longer questionable: ConnectInterfaces() is expected to be called multiple times for a file that exports multiple interfaces.
		// Warning("APPSYSTEM: ConnectInterfaces() was called twice for the same DLL.\nThis is expected behavior in building reslists, but questionable otherwise.\n");
		for ( int i = 0; i < nFactoryCount; ++i )
		{
			for ( int j = 0; j < ARRAYSIZE( g_pInterfaceGlobals ); ++j )
			{
				ReconnectInterface( pFactoryList[i], g_pInterfaceGlobals[j].m_pInterfaceName, (void**)g_pInterfaceGlobals[j].m_ppGlobal );
			}
		}
	}
	++s_nConnectionCount;
}

void DisconnectInterfaces()
{
	Assert( s_nConnectionCount > 0 );
	if ( --s_nConnectionCount < 0 )
		return;

	for ( int i = 0; i < s_nRegistrationCount; ++i )
	{
		if ( s_pConnectionRegistration[i].m_nConnectionPhase != s_nConnectionCount )
			continue;

		// Disconnect!
		*(void**)(s_pConnectionRegistration[i].m_ppGlobalStorage) = nullptr;
	}
}


//-----------------------------------------------------------------------------
// Reloads an interface
//-----------------------------------------------------------------------------
void ReconnectInterface( CreateInterfaceFn factory, const char *pInterfaceName )
{
	for ( int i = 0; i < ARRAYSIZE( g_pInterfaceGlobals ); ++i )
	{
		if ( strcmp( g_pInterfaceGlobals[i].m_pInterfaceName, pInterfaceName ) )
			continue;		
		ReconnectInterface( factory, g_pInterfaceGlobals[i].m_pInterfaceName, (void**)g_pInterfaceGlobals[i].m_ppGlobal );
	}
}
