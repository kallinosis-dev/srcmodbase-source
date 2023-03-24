//============ Copyright (c) Valve Corporation, All rights reserved. ==========
//
//=============================================================================


#ifndef FBXSYSTEM_H
#define FBXSYSTEM_H


#if COMPILER_MSVC
#pragma once
#endif


// Valve includes
#include "fbxsystem/ifbxsystem.h"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CFbxSystem : public CBaseAppSystem< IFbxSystem >
{
	typedef CBaseAppSystem< IFbxSystem > BaseClass;

public:
	CFbxSystem();
	virtual ~CFbxSystem();

	// From CBaseAppSystem
	virtual InitReturnVal_t Init() override;
	virtual void Shutdown() override;
	virtual AppSystemTier_t GetTier() override { return APP_SYSTEM_TIER2; }
	virtual bool IsSingleton() override{ return false; }

	// From IFbxSystem
	virtual FbxManager *GetFbxManager();

private:
	FbxManager *m_pFbxManager;

};

#endif // FBXSYSTEM_H
