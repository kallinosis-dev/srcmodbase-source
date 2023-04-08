//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERBROWSER_H
#define SERVERBROWSER_H
#ifdef _WIN32
#pragma once
#endif

class CServerBrowserDialog;

//-----------------------------------------------------------------------------
// Purpose: Handles the UI and pinging of a half-life game server list
//-----------------------------------------------------------------------------
class CServerBrowser : public IServerBrowser, public IVGuiModule
{
public:
	CServerBrowser();
	~CServerBrowser() override;

	// IVGui module implementation
	bool Initialize(CreateInterfaceFn *factorylist, int numFactories) override;
	bool PostInitialize(CreateInterfaceFn *modules, int factoryCount) override;
	vgui::VPANEL GetPanel() override;
	bool Activate() override;
	bool IsValid() override;
	void Shutdown() override;
	void Deactivate() override;
	void Reactivate() override;
	void SetParent(vgui::VPANEL parent) override;

	// IServerBrowser implementation
	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	bool JoinGame( uint32 unGameIP, uint16 usGamePort ) override;
	bool JoinGame( uint64 ulSteamIDFriend ) override;

	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	bool OpenGameInfoDialog( uint64 ulSteamIDFriend ) override;

	// forces the game info dialog closed
	void CloseGameInfoDialog( uint64 ulSteamIDFriend ) override;

	// closes all the game info dialogs
	void CloseAllGameInfoDialogs() override;

	// methods
	virtual void CreateDialog();
	virtual void Open();

	// true if the user can't play a game
	bool IsVACBannedFromGame( int nAppID );


private:
	vgui::DHANDLE<CServerBrowserDialog> m_hInternetDlg;
};

// singleton accessor
CServerBrowser &ServerBrowser();

#ifndef NO_STEAM
class CSteamAPIContext;
extern CSteamAPIContext *steamapicontext;
#endif


#endif // SERVERBROWSER_H
