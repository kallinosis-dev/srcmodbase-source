//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEUISYSTEMSURFACE_H
#define GAMEUISYSTEMSURFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "igameuisystemmgr.h"
#include "materialsystem/imaterialsystem.h"
#include "vgui_surfacelib/ifontsurface.h" 
#include "tier1/utldict.h"
#include "rendersystem/irenderdevice.h"

class CFontTextureCache;
class KeyValues;

#ifdef CreateFont
#undef CreateFont
#endif

//-----------------------------------------------------------------------------
// This class is the interface to the font and font texture, systems.
// Load fonts given by schemes into the systems using this class.
//-----------------------------------------------------------------------------
class CGameUISystemSurface : public IGameUISystemSurface
{

public:

	CGameUISystemSurface();
	~CGameUISystemSurface();

	InitReturnVal_t Init() override;
	void Shutdown() override;

	void PrecacheFontCharacters( FontHandle_t font, wchar_t const* pCharacterString = nullptr) override;

	FontHandle_t CreateFont() override;
	bool SetFontGlyphSet( FontHandle_t font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0 ) override;
	int GetFontTall( FontHandle_t font ) override;
	void GetCharABCwide( FontHandle_t font, int ch, int &a, int &b, int &c ) override;
	int GetCharacterWidth( FontHandle_t font, int ch ) override;
	const char *GetFontName( FontHandle_t font ) override;

	bool AddCustomFontFile( const char *fontFileName ) override;

	// Helper fxns for loading bitmap fonts
	bool AddBitmapFontFile( const char *fontFileName ) override;
	void SetBitmapFontName( const char *pName, const char *pFontFilename ) override;
	const char *GetBitmapFontName( const char *pName ) override;
	bool SetBitmapFontGlyphSet( FontHandle_t font, const char *windowsFontName, float scalex, float scaley, int flags) override;

	void ClearTemporaryFontCache( void ) override;

	// Causes fonts to get reloaded, etc.
	void ResetFontCaches() override;

	bool SupportsFontFeature( FontFeature_t feature ) override;

	void DrawSetTextureRGBA( int id, const unsigned char* rgba, int wide, int tall ){}
	void DrawSetTextureRGBAEx( int id, const unsigned char* rgba, int wide, int tall, ImageFormat format ){}

	bool GetUnicodeCharRenderPositions( FontCharRenderInfo& info, Vector2D *pPositions ) override;
 	IMaterial *GetTextureForChar( FontCharRenderInfo &info, float **texCoords ) override;
	IMaterial *GetTextureAndCoordsForChar( FontCharRenderInfo &info, float *texCoords ) override;

	// Used for debugging.
	void DrawFontTexture( int textureId, int xPos, int yPos ) override;
	void DrawFontTexture( IRenderContext *pRenderContext, int textureId, int xPos, int yPos ) override;
	
	IMaterial *GetMaterial( int textureId ) override;
	HRenderTexture GetTextureHandle( int textureId ) override;

	void GetProportionalBase( int &width, int &height ) { width = BASE_WIDTH; height = BASE_HEIGHT; }

	void SetLanguage( const char *pLanguage ) override;
	const char *GetLanguage() override;


private:
	enum { BASE_HEIGHT = 480, BASE_WIDTH = 640 };
		
	bool m_bIsInitialized;
	
	CUtlVector< CUtlSymbol >	m_CustomFontFileNames;
	CUtlVector< CUtlSymbol >	m_BitmapFontFileNames;
	CUtlDict< int, int >		m_BitmapFontFileMapping;

};

extern CGameUISystemSurface *g_pGameUISystemSurface;




#endif // GAMEUISYSTEMSURFACE_H
