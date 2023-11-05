//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SND_MP3_SOURCE_H
#define SND_MP3_SOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "snd_audio_source.h"
#include "snd_wave_data.h"
#include "snd_sfx.h"

class IWaveData;
class CAudioMixer;

abstract_class CAudioSourceMP3 : public CAudioSource
{
public:

	CAudioSourceMP3( CSfxTable *pSfx );
	CAudioSourceMP3( CSfxTable *pSfx, CAudioSourceCachedInfo *info );
	~CAudioSourceMP3() override;

	// Create an instance (mixer) of this audio source
	CAudioMixer			*CreateMixer( int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo, SoundError &soundError
#ifdef WITH_PHONON
		, struct hrtf_info_t* pHRTFVec
#endif
	) override = 0;

	int					GetType( void ) override;
	void				GetCacheData( CAudioSourceCachedInfo *info ) override;

	// Provide samples for the mixer. You can point pData at your own data, or if you prefer to copy the data,
	// you can copy it into copyBuf and set pData to copyBuf.
	int					GetOutputData( void **pData, int64 samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) override = 0;

	int					SampleRate( void ) override { return m_sampleRate; }

	// Returns true if the source is a voice source.
	// This affects the voice_overdrive behavior (all sounds get quieter when
	// someone is speaking).
	bool				IsVoiceSource() override { return false; }
	bool				IsPlayerVoice() override { return false; }
	int					SampleSize( void ) override { return 1; }

	// Total number of samples in this source.  NOTE: Some sources are infinite (mic input), they should return
	// a count equal to one second of audio at their current rate.
	int					SampleCount( void ) override { return m_dataSize; }

	int					Format() override { return 0; }
	int					DataSize( void ) override { return 0; }

	bool				IsLooped( void ) override { return false; }
	bool				IsStereoWav( void ) override { return false; }
	bool				IsStreaming( void ) override { return false; }
	int					GetCacheStatus( void ) override { return AUDIO_IS_LOADED; }
	void				CacheLoad( void ) override {}
	void				CacheUnload( void ) override {}
	CSentence			*GetSentence( void ) override { return nullptr; }
	int					GetQuality( void ) override { return 0; }

	int					ZeroCrossingBefore( int sample ) override { return sample; }
	int					ZeroCrossingAfter( int sample ) override { return sample; }
	
	// mixer's references
	void				ReferenceAdd( CAudioMixer *pMixer ) override;
	void				ReferenceRemove( CAudioMixer *pMixer ) override;
	// check reference count, return true if nothing is referencing this
	bool				CanDelete( void ) override;

	bool				IsAsyncLoad() override;

	void				CheckAudioSourceCache() override;

	char const			*GetFileName( char *pOutBuf, size_t bufLen ) override;

	void				SetPlayOnce( bool isPlayOnce ) override { m_bIsPlayOnce = isPlayOnce; }
	bool				IsPlayOnce() override { return m_bIsPlayOnce; }

	void				SetSentenceWord( bool bIsWord ) override { m_bIsSentenceWord = bIsWord; }
	bool				IsSentenceWord() override { return m_bIsSentenceWord; }

	int					SampleToStreamPosition( int samplePosition ) override { return 0; }
	int					StreamToSamplePosition( int streamPosition ) override { return 0; }

protected:

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : byte
	//-----------------------------------------------------------------------------
	inline byte *GetCachedDataPointer()
	{
		VPROF("CAudioSourceMP3::GetCachedDataPointer");

		CAudioSourceCachedInfo *info = m_AudioCacheHandle.Get( CAudioSource::AUDIO_SOURCE_MP3, m_pSfx->IsPrecachedSound(), m_pSfx, &m_nCachedDataSize );
		if ( !info )
		{
			Assert( !"CAudioSourceMP3::GetCachedDataPointer info == NULL" );
			return nullptr;
		}

		return (byte *)info->CachedData();
	}

	CAudioSourceCachedInfoHandle_t m_AudioCacheHandle;
	int				m_nCachedDataSize;

protected:
	bool						GetStartupData();

	CSfxTable		*m_pSfx;
	int				m_sampleRate;
	int				m_dataSize;
	int				m_dataStart;
	int				m_refCount;
	bool			m_bIsPlayOnce : 1;
	bool			m_bIsSentenceWord : 1;
};

//-----------------------------------------------------------------------------
// Purpose: Streaming MP3 file
//-----------------------------------------------------------------------------
class CAudioSourceStreamMP3 : public CAudioSourceMP3, public IWaveStreamSource
{
public:
	CAudioSourceStreamMP3( CSfxTable *pSfx );
	CAudioSourceStreamMP3( CSfxTable *pSfx, CAudioSourceCachedInfo *info );
	~CAudioSourceStreamMP3() override {}

	bool			IsStreaming( void ) override { return true; }
	bool			IsStereoWav(void) override { return false; }
	CAudioMixer		*CreateMixer(int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo, SoundError &soundError
#ifdef WITH_PHONON
		, struct hrtf_info_t* pHRTFVec
#endif
	) override;
	int				GetOutputData( void **pData, int64 samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) override;

	// IWaveStreamSource
	int64 UpdateLoopingSamplePosition( int64 samplePosition ) override
	{
		return samplePosition;
	}

	void UpdateSamples( char *pData, int sampleCount ) override {}

	int	GetLoopingInfo( int *pLoopBlock, int *pNumLeadingSamples, int *pNumTrailingSamples ) override
	{
		return 0;
	}

	void Prefetch() override;

private:
	CAudioSourceStreamMP3( const CAudioSourceStreamMP3 & ); // not implemented, not accessible
};

class CAudioSourceMP3Cache : public CAudioSourceMP3
{
public:
	CAudioSourceMP3Cache( CSfxTable *pSfx );
	CAudioSourceMP3Cache( CSfxTable *pSfx, CAudioSourceCachedInfo *info );
	~CAudioSourceMP3Cache( void ) override;

	int						GetCacheStatus( void ) override;
	void					CacheLoad( void ) override;
	void					CacheUnload( void ) override;
	// NOTE: "samples" are bytes for MP3
	int						GetOutputData( void **pData, int64 samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) override;
	CAudioMixer				*CreateMixer( int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo, SoundError &soundError
#ifdef WITH_PHONON
		, struct hrtf_info_t* pHRTFVec
#endif
	) override;

	void			Prefetch() override {}

protected:
	virtual char			*GetDataPointer( void );
	WaveCacheHandle_t		m_hCache;

private:
	CAudioSourceMP3Cache( const CAudioSourceMP3Cache & );
};

bool Audio_IsMP3( const char *pName );
CAudioSource *Audio_CreateStreamedMP3( CSfxTable *pSfx );
CAudioSource *Audio_CreateMemoryMP3( CSfxTable *pSfx );
CAudioMixer *CreateMP3Mixer( IWaveData *data, int *pSampleRate );

#endif // SND_MP3_SOURCE_H
