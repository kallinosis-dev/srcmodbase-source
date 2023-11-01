//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef MASTER_H
#define MASTER_H
#ifdef _WIN32
#pragma once
#endif

#ifndef NO_STEAM
// On by default, but when this is false, we disable the master server updater.
extern bool g_bEnableMasterServerUpdater;

extern ConVar	sv_search_key;
void Heartbeat_f();
#endif

#endif // MASTER_H
