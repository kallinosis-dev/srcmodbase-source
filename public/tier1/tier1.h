//===== Copyright � 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER1_H
#define TIER1_H

#if defined( _WIN32 )
#pragma once
#endif

#include "appframework/iappsystem.h"
#include "tier1/convar.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Call this to connect to/disconnect from all tier 1 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectTier1Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount );
void DisconnectTier1Libraries();


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier1AppSystem : public CTier0AppSystem< IInterface >
{
	typedef CTier0AppSystem< IInterface > BaseClass;

public:
	bool Connect( CreateInterfaceFn factory ) override
	{
		if ( !BaseClass::Connect( factory ) )
			return false;

		ConnectTier1Libraries( &factory, 1 );
		return true;
	}

	void Disconnect() override
	{
		DisconnectTier1Libraries();
		BaseClass::Disconnect();
	}

	InitReturnVal_t Init() override
	{
		InitReturnVal_t nRetVal = BaseClass::Init();
		if ( nRetVal != INIT_OK )
			return nRetVal;

		if ( g_pCVar )
		{
			ConVar_Register( ConVarFlag );
		}
		return INIT_OK;
	}

	void Shutdown() override
	{
		if ( g_pCVar )
		{
			ConVar_Unregister( );
		}
		BaseClass::Shutdown( );
	}

	AppSystemTier_t GetTier() override
	{
		return APP_SYSTEM_TIER1;
	}
};


#endif // TIER1_H

