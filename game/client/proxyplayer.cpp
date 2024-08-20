//========= Copyright  1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include <keyvalues.h>
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "functionproxy.h"
#include "c_cs_player.h"
#include "weapon_csbase.h"
#include "predicted_viewmodel.h"
#include "cs_client_gamestats.h"

#include "imaterialproxydict.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Returns the proximity of the player to the entity
//-----------------------------------------------------------------------------

class CPlayerProximityProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerProximityProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 0.002 );
	return true;
}

void CPlayerProximityProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the distance between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Vector delta;
	VectorSubtract( pEntity->WorldSpaceCenter(), pPlayer->WorldSpaceCenter(), delta );

	Assert( m_pResult );
	SetFloatResult( delta.Length() * m_Factor );
}

EXPOSE_MATERIAL_PROXY( CPlayerProximityProxy, PlayerProximity );


//-----------------------------------------------------------------------------
// Returns true if the player's team matches that of the entity the proxy material is attached to
//-----------------------------------------------------------------------------

class CPlayerTeamMatchProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
};

bool CPlayerTeamMatchProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	return true;
}

void CPlayerTeamMatchProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the distance between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Assert( m_pResult );
	SetFloatResult( (pEntity->GetTeamNumber() == pPlayer->GetTeamNumber()) ? 1.0 : 0.0 );
}

EXPOSE_MATERIAL_PROXY( CPlayerTeamMatchProxy, PlayerTeamMatch );


//-----------------------------------------------------------------------------
// Returns the player view direction
//-----------------------------------------------------------------------------
class CPlayerViewProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerViewProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 2 );
	return true;
}

void CPlayerViewProxy::OnBind( void *pC_BaseEntity )
{
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Vector delta;
	VectorSubtract( pEntity->WorldSpaceCenter(), pPlayer->WorldSpaceCenter(), delta );
	VectorNormalize( delta );

	Vector forward;
	AngleVectors( pPlayer->GetAbsAngles(), &forward );

	Assert( m_pResult );
	SetFloatResult( DotProduct( forward, delta ) * m_Factor );
}

EXPOSE_MATERIAL_PROXY( CPlayerViewProxy, PlayerView );


//-----------------------------------------------------------------------------
// Returns the player speed
//-----------------------------------------------------------------------------
class CPlayerSpeedProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerSpeedProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	m_Factor = pKeyValues->GetFloat( "scale", 0.005 );
	return true;
}

void CPlayerSpeedProxy::OnBind( void *pC_BaseEntity )
{
	// Find the player speed....
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	Assert( m_pResult );
	SetFloatResult( pPlayer->GetLocalVelocity().Length() * m_Factor );
}

EXPOSE_MATERIAL_PROXY( CPlayerSpeedProxy, PlayerSpeed );


//-----------------------------------------------------------------------------
// Returns the player position
//-----------------------------------------------------------------------------
class CPlayerPositionProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	float	m_Factor;
};

bool CPlayerPositionProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;
	  
	m_Factor = pKeyValues->GetFloat( "scale", 0.005 );
	return true;
}

void CPlayerPositionProxy::OnBind( void *pC_BaseEntity )
{
	// Find the player speed....
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	// This is actually a vector...
	Assert( m_pResult );
	Vector res;
	VectorMultiply( pPlayer->WorldSpaceCenter(), m_Factor, res ); 
	m_pResult->SetVecValue( res.Base(), 3 );
}

EXPOSE_MATERIAL_PROXY( CPlayerPositionProxy, PlayerPosition );


//-----------------------------------------------------------------------------
// Returns the entity speed
//-----------------------------------------------------------------------------
class CEntitySpeedProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CEntitySpeedProxy::OnBind( void *pC_BaseEntity )
{
	// Find the view angle between the player and this entity....
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

	Assert( m_pResult );
	m_pResult->SetFloatValue( pEntity->GetLocalVelocity().Length() );
}

EXPOSE_MATERIAL_PROXY( CEntitySpeedProxy, EntitySpeed );


//-----------------------------------------------------------------------------
// Returns a random # from 0 - 1 specific to the entity it's applied to
//-----------------------------------------------------------------------------
class CEntityRandomProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_Factor;
};

bool CEntityRandomProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Factor.Init( pMaterial, pKeyValues, "scale", 1 ))
		return false;

	return true;
}

void CEntityRandomProxy::OnBind( void *pC_BaseEntity )
{
	// Find the view angle between the player and this entity....
	if (!pC_BaseEntity)
		return;

	// Find the view angle between the player and this entity....
	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

	Assert( m_pResult );
	m_pResult->SetFloatValue( pEntity->ProxyRandomValue() * m_Factor.GetFloat() );
}

EXPOSE_MATERIAL_PROXY( CEntityRandomProxy, EntityRandom );

#ifdef IRONSIGHT
//-----------------------------------------------------------------------------
// IronSightAmount proxy
//-----------------------------------------------------------------------------
class CIronSightAmountProxy : public CResultProxy
{
public:
	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	virtual void OnBind(void *pC_BaseEntity);
private:
	bool bInvert;
};


bool CIronSightAmountProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	if (!CResultProxy::Init(pMaterial, pKeyValues))
		return false;

	bInvert = false;
	CFloatInput	m_flInvert;
	if ( m_flInvert.Init( pMaterial, pKeyValues, "invert" ) )
		bInvert = ( m_flInvert.GetFloat() > 0 );

	return true;
}

void CIronSightAmountProxy::OnBind(void *pC_BaseEntity)
{

	if (!pC_BaseEntity)
		return;
	
	C_BaseEntity *pEntity = BindArgToEntity(pC_BaseEntity);
	if (pEntity)
	{
		C_BaseViewModel *pViewModel = dynamic_cast<C_BaseViewModel*>(pEntity);
		if (pViewModel)
		{
			C_CSPlayer *pPlayer = ToCSPlayer(pViewModel->GetPredictionOwner());
			if (pPlayer)
			{
				CWeaponCSBase *pWeapon = pPlayer->GetActiveCSWeapon();
				if ( pWeapon && pWeapon->GetIronSightController() )
				{
					if ( bInvert )
					{
						SetFloatResult(Bias( 1.0f - pWeapon->GetIronSightController()->GetIronSightAmount(), 0.2f));
					}
					else
					{
						SetFloatResult(Bias(pWeapon->GetIronSightController()->GetIronSightAmount(), 0.2f));
					}
				}
			}
		}
	}

}


EXPOSE_MATERIAL_PROXY(CIronSightAmountProxy, IronSightAmount);
#endif //IRONSIGHT


//-----------------------------------------------------------------------------
// CrosshairColor proxy
//-----------------------------------------------------------------------------
extern ConVar cl_crosshaircolor_r;
extern ConVar cl_crosshaircolor_g;
extern ConVar cl_crosshaircolor_b;
class CCrossHairColorProxy : public CResultProxy
{
public:
	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	virtual void OnBind(void *pC_BaseEntity);

	Vector m_vecLocalCrossHairColor;
};

bool CCrossHairColorProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	if (!CResultProxy::Init(pMaterial, pKeyValues))
		return false;
	m_vecLocalCrossHairColor.Init();
	return true;
}

void CCrossHairColorProxy::OnBind(void *pC_BaseEntity)
{
	if ( m_vecLocalCrossHairColor.x != cl_crosshaircolor_r.GetFloat() || 
		 m_vecLocalCrossHairColor.y != cl_crosshaircolor_g.GetFloat() || 
		 m_vecLocalCrossHairColor.z != cl_crosshaircolor_b.GetFloat() )
	{

		m_vecLocalCrossHairColor.x = cl_crosshaircolor_r.GetFloat();
		m_vecLocalCrossHairColor.y = cl_crosshaircolor_g.GetFloat();
		m_vecLocalCrossHairColor.z = cl_crosshaircolor_b.GetFloat();

		SetVecResult(   (float)m_vecLocalCrossHairColor.x * 0.0039,
						(float)m_vecLocalCrossHairColor.y * 0.0039,
						(float)m_vecLocalCrossHairColor.z * 0.0039, 1);
	}
}

EXPOSE_MATERIAL_PROXY(CCrossHairColorProxy, CrossHairColor);


float g_flEconInspectPreviewTime = 0;

class CEconInspectPreviewTimeProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CEconInspectPreviewTimeProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if ( !CResultProxy::Init( pMaterial, pKeyValues ) )
		return false;

	return true;
}

void CEconInspectPreviewTimeProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );
	SetFloatResult( gpGlobals->curtime - g_flEconInspectPreviewTime );
}

EXPOSE_MATERIAL_PROXY( CEconInspectPreviewTimeProxy, EconInspectPreviewTime );

