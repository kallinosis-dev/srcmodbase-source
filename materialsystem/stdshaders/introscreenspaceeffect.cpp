//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "screenspaceeffect_vs20.inc"
#include "IntroScreenSpaceEffect_ps20.inc"
#include "IntroScreenSpaceEffect_ps20b.inc"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


BEGIN_VS_SHADER_FLAGS( IntroScreenSpaceEffect, "Help for IntroScreenSpaceEffect", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( MODE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENABLESRGB, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );

		if ( !params[ENABLESRGB]->IsDefined() )
		{
			params[ENABLESRGB]->SetIntValue( 0 );
		}
	}

	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		return nullptr;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			if ( params[ENABLESRGB]->GetIntValue() )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBWrite( true );
			}

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, nullptr, 0 );

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // GL always goes the ps2b way for this shader, even on "ps20" parts
			{
				DECLARE_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( LINEAR_TO_SRGB, false );
				SET_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps20 );
				SET_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps20 );
			}

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_BINDFLAGS_SRGBREAD, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_BINDFLAGS_SRGBREAD, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // GL always goes the ps2b way for this shader, even on "ps20" parts
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( MODE, params[MODE]->GetIntValue() );
				SET_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( MODE, params[MODE]->GetIntValue() );
				SET_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps20 );
			}

			SetPixelShaderConstant( 0, ALPHA );
		}
		Draw();
	}
END_SHADER
