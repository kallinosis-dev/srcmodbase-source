//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HLTVCLIENTSTATE_H
#define HLTVCLIENTSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseclientstate.h"

#ifdef WITH_HLTV
class CClientFrame;
class CHLTVServer;
class CMsg_CVars;

extern ConVar tv_name;

class CHLTVClientState : public CBaseClientState
{

friend class CHLTVServer;

public:
	CHLTVClientState( CHLTVServer *pHltvServer );
	virtual ~CHLTVClientState();

public:
	
	const char *GetCDKeyHash() { return "HLTVHLTVHLTVHLTVHLTVHLTVHLTVHLTV"; }; // haha
	bool SetSignonState ( int state, int count, const CNETMsg_SignonState *msg ) override;
	void SendClientInfo( void );
	void PacketEnd( void );
	void Clear( void );
	void RunFrame ( void );
	void InstallStringTableCallback( char const *tableName );
	virtual bool HookClientStringTable( char const *tableName );
	virtual const char *GetClientName() { return tv_name.GetString(); }

	void ConnectionCrashed( const char * reason );
	void ConnectionClosing( const char * reason );
	virtual void Disconnect( bool bShowMainMenu = true ) override;
	int GetConnectionRetryNumber() const;

	void ReadEnterPVS( CEntityReadInfo &u );
	void ReadLeavePVS( CEntityReadInfo &u );
	void ReadDeltaEnt( CEntityReadInfo &u );
	void ReadPreserveEnt( CEntityReadInfo &u );
	void ReadDeletions( CEntityReadInfo &u );
	void ReadPacketEntities( CEntityReadInfo &u );

	void CopyNewEntity( CEntityReadInfo &u,	int iClass,	int iSerialNum );

	virtual bool IsClientStateTv() const { return true; }

public: // IServerMessageHandlers

	virtual bool NETMsg_StringCmd( const CNETMsg_StringCmd& msg ) override;
	virtual bool NETMsg_SetConVar( const CNETMsg_SetConVar& msg ) override;
	virtual bool NETMsg_PlayerAvatarData( const CNETMsg_PlayerAvatarData& msg ) override;
	virtual bool SVCMsg_ServerInfo( const CSVCMsg_ServerInfo& msg ) override;
		
	virtual bool SVCMsg_ClassInfo( const CSVCMsg_ClassInfo& msg ) override;
	virtual bool SVCMsg_SetView( const CSVCMsg_SetView& msg ) override;
	virtual bool SVCMsg_VoiceInit( const CSVCMsg_VoiceInit& msg ) override;
	virtual bool SVCMsg_VoiceData( const CSVCMsg_VoiceData& msg ) override;
	virtual bool SVCMsg_FixAngle( const CSVCMsg_FixAngle& msg ) override;
	virtual bool SVCMsg_Prefetch( const CSVCMsg_Prefetch& msg ) override;
	virtual bool SVCMsg_CrosshairAngle( const CSVCMsg_CrosshairAngle& msg ) override;
	virtual bool SVCMsg_BSPDecal( const CSVCMsg_BSPDecal& msg ) override;
	virtual bool SVCMsg_Menu( const CSVCMsg_Menu& msg ) override;
	virtual bool SVCMsg_UserMessage( const CSVCMsg_UserMessage& msg ) override;
	virtual bool SVCMsg_PaintmapData( const CSVCMsg_PaintmapData& msg ) override;
	virtual bool SVCMsg_GameEvent( const CSVCMsg_GameEvent& msg ) override;
	virtual bool SVCMsg_GameEventList( const CSVCMsg_GameEventList &msg ) override;
	virtual bool SVCMsg_TempEntities( const CSVCMsg_TempEntities &msg ) override;
	virtual bool SVCMsg_PacketEntities( const CSVCMsg_PacketEntities &msg ) override;
	virtual bool SVCMsg_Sounds( const CSVCMsg_Sounds& msg ) override;
	virtual bool SVCMsg_EntityMsg( const CSVCMsg_EntityMsg& msg) override;
	virtual bool SVCMsg_EncryptedData( const CSVCMsg_EncryptedData& msg ) override;

public:
	void SendPacket();
	void UpdateStats();
	void SetLocalInfoConvarsForUpstreamConnection( CMsg_CVars &cvars, bool bMaxSlots = false );

	CClientFrame	*m_pNewClientFrame; // not NULL if we just got a packet with a new entity frame
	CClientFrame	*m_pCurrentClientFrame; // NULL or pointer to last entity frame
	bool			m_bSaveMemory; //compress data as much as possible to keep whole demos in memory
	float			m_fNextSendUpdateTime;
	CHLTVServer		*m_pHLTV;	// HLTV server this client state belongs too.

	int				eventid_hltv_status;
	int				eventid_hltv_title;
};
#endif

#endif // HLTVCLIENTSTATE_H
