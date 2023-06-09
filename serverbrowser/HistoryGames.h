//====== Copyright � 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef HISTORYGAMES_H
#define HISTORYGAMES_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: History of all the servers joined
//-----------------------------------------------------------------------------
class CHistoryGames : public CBaseGamesPage
{
	DECLARE_CLASS_SIMPLE( CHistoryGames, CBaseGamesPage );

public:
	CHistoryGames(vgui::Panel *parent);
	~CHistoryGames() override;

	// favorites list, loads/saves into keyvalues
	void LoadHistoryList();


	// IGameList handlers
	// returns true if the game list supports the specified ui elements
	bool SupportsItem(InterfaceItem_e item) override;

#ifndef NO_STEAM
	// called when the current refresh list is complete
	virtual void RefreshComplete( HServerListRequest hReq, EMatchMakingServerResponse response );
#endif

	void SetRefreshOnReload() { m_bRefreshOnListReload = true; }

private:
	// context menu message handlers
	MESSAGE_FUNC_INT( OnOpenContextMenu, "OpenContextMenu", itemID );
	MESSAGE_FUNC( OnRemoveFromHistory, "RemoveFromHistory" );

	bool m_bRefreshOnListReload;
};


#endif // HISTORYGAMES_H
