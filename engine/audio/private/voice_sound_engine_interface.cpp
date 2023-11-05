//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "audio_pch.h"
#include <assert.h>
#include "voice.h"
#include "ivoicecodec.h"

#if defined( _X360 )
#include "xauddefs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------- //
// CAudioSourceVoice.
// This feeds the data from an incoming voice channel (a guy on the server
// who is speaking) into the sound engine.
// ------------------------------------------------------------------------- //

class CAudioSourceVoice : public CAudioSourceWave
{
public:
	CAudioSourceVoice(CSfxTable* pSfx, int iEntity);
	~CAudioSourceVoice() override;

	int GetType(void) override
	{
		return AUDIO_SOURCE_VOICE;
	}

	void GetCacheData(CAudioSourceCachedInfo* info) override
	{
		Assert(0);
	}


	CAudioMixer* CreateMixer(int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo, SoundError& soundError
#ifdef WITH_PHONON
	    , struct hrtf_info_t* pHRTFVec
#endif
	) override;
	int GetOutputData(void** pData, int64 samplePosition, int sampleCount,
	                  char copyBuf[AUDIOSOURCE_COPYBUF_SIZE]) override;
	int SampleRate(void) override;

	// Sample size is in bytes.  It will not be accurate for compressed audio.  This is a best estimate.
	// The compressed audio mixers understand this, but in general do not assume that SampleSize() * SampleCount() = filesize
	// or even that SampleSize() is 100% accurate due to compression.
	int SampleSize(void) override;

	// Total number of samples in this source.  NOTE: Some sources are infinite (mic input), they should return
	// a count equal to one second of audio at their current rate.
	int SampleCount(void) override;

	bool IsVoiceSource() override { return true; }
	bool IsPlayerVoice() override { return true; }

	bool IsLooped() override { return false; }
	bool IsStreaming() override { return true; }
	bool IsStereoWav() override { return false; }
	int GetCacheStatus() override { return AUDIO_IS_LOADED; }

	void CacheLoad() override
	{
	}

	void CacheUnload() override
	{
	}

	CSentence* GetSentence() override { return nullptr; }
	int GetQuality() override { return 0; }

	int ZeroCrossingBefore(int sample) override { return sample; }
	int ZeroCrossingAfter(int sample) override { return sample; }

	// mixer's references
	void ReferenceAdd(CAudioMixer* pMixer) override;
	void ReferenceRemove(CAudioMixer* pMixer) override;

	// check reference count, return true if nothing is referencing this
	bool CanDelete() override;

	void Prefetch() override
	{
	}

	// Nothing, not a cache object...
	void CheckAudioSourceCache() override
	{
	}

private:
	class CWaveDataVoice : public IWaveData
	{
	public:
		CWaveDataVoice(CAudioSourceWave& source) : m_source(source)
		{
		}

		~CWaveDataVoice(void) override
		{
		}

		CAudioSource& Source(void) override
		{
			return m_source;
		}

		// this file is in memory, simply pass along the data request to the source
		int ReadSourceData(void** pData, int64 sampleIndex, int sampleCount,
		                   char copyBuf[AUDIOSOURCE_COPYBUF_SIZE]) override
		{
			return m_source.GetOutputData(pData, sampleIndex, sampleCount, copyBuf);
		}

		bool IsReadyToMix() override
		{
			return true;
		}

		void UpdateLoopPosition(int nLoopPosition) override
		{
			// Should not be necessary in this implementation...
			Assert(false);
		}

	private:
		CAudioSourceWave& m_source; // pointer to source
	};

private:
	CAudioSourceVoice(const CAudioSourceVoice&);

	// Which entity's voice this is for.
	int m_iChannel;

	// How many mixers are referencing us.
	int m_refCount;
};


// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

// The format we sample voice in.
extern WAVEFORMATEX g_VoiceSampleFormat;

class CVoiceSfx : public CSfxTable
{
public:
	virtual const char* getname(char* pBuf, size_t bufLen)
	{
		const char* pName = "?VoiceSfx";
		V_strncpy(pBuf, pName, bufLen);
		return pBuf;
	}
};

static CVoiceSfx g_CVoiceSfx[VOICE_NUM_CHANNELS];

static float g_VoiceOverdriveDuration = 0;
static bool g_bVoiceOverdriveOn = false;

// When voice is on, all other sounds are decreased by this factor.
static ConVar voice_overdrive("voice_overdrive", "2");
static ConVar voice_overdrivefadetime("voice_overdrivefadetime", "0.4");
// How long it takes to fade in and out of the voice overdrive.

// The sound engine uses this to lower all sound volumes.
// All non-voice sounds are multiplied by this and divided by 256.
int g_SND_VoiceOverdriveInt = 256;


extern int Voice_SamplesPerSec();
extern int Voice_AvgBytesPerSec();

// ----------------------------------------------------------------------------- //
// CAudioSourceVoice implementation.
// ----------------------------------------------------------------------------- //

CAudioSourceVoice::CAudioSourceVoice(CSfxTable* pSfx, int iChannel)
	: CAudioSourceWave(pSfx)
{
	m_iChannel = iChannel;
	m_refCount = 0;

	WAVEFORMATEX tmp = g_VoiceSampleFormat;
	tmp.nSamplesPerSec = Voice_SamplesPerSec();
	tmp.nAvgBytesPerSec = Voice_AvgBytesPerSec();
	Init((char*)&tmp, sizeof(tmp));
	m_sampleCount = tmp.nSamplesPerSec;
}

CAudioSourceVoice::~CAudioSourceVoice()
{
	Voice_OnAudioSourceShutdown(m_iChannel);
}

CAudioMixer* CAudioSourceVoice::CreateMixer(int initialStreamPosition, int skipInitialSamples, bool bUpdateDelayForChoreo, SoundError& soundError
#ifdef WITH_PHONON
	, struct hrtf_info_t* pHRTFVec
#endif
)
{
	soundError = SE_OK;
	CWaveDataVoice* pVoice = new CWaveDataVoice(*this);
	if (!pVoice)
	{
		Assert(false);
		soundError = SE_CANT_CREATE_MIXER;
		return nullptr;
	}

	CAudioMixer* pMixer = CreateWaveMixer(pVoice, WAVE_FORMAT_PCM, 1, BYTES_PER_SAMPLE * 8, initialStreamPosition,
	                                      skipInitialSamples, bUpdateDelayForChoreo);
	if (!pMixer)
	{
		delete pVoice;
		soundError = SE_CANT_CREATE_MIXER;
		return nullptr;
	}

	ReferenceAdd(pMixer);
	return pMixer;
}

int CAudioSourceVoice::GetOutputData(void** pData, int64 samplePosition, int sampleCount,
                                     char copyBuf[AUDIOSOURCE_COPYBUF_SIZE])
{
	int nSamplesGotten = Voice_GetOutputData(
		m_iChannel,
		copyBuf,
		AUDIOSOURCE_COPYBUF_SIZE,
		(int)samplePosition,
		sampleCount);

	// If there weren't enough bytes in the received data channel, pad it with zeros.
	if (nSamplesGotten < sampleCount)
	{
		if (copyBuf != nullptr)
		{
			memset(&copyBuf[nSamplesGotten], 0, (sampleCount - nSamplesGotten) * BYTES_PER_SAMPLE);
		}
		nSamplesGotten = sampleCount;
	}

	*pData = copyBuf;
	return nSamplesGotten;
}

int CAudioSourceVoice::SampleRate()
{
	return Voice_SamplesPerSec();
}

int CAudioSourceVoice::SampleSize()
{
	return BYTES_PER_SAMPLE;
}

int CAudioSourceVoice::SampleCount()
{
	return Voice_SamplesPerSec();
}

void CAudioSourceVoice::ReferenceAdd(CAudioMixer* pMixer)
{
	m_refCount++;
}

void CAudioSourceVoice::ReferenceRemove(CAudioMixer* pMixer)
{
	m_refCount--;
	if (m_refCount <= 0)
		delete this;
}

bool CAudioSourceVoice::CanDelete()
{
	return m_refCount == 0;
}


// ----------------------------------------------------------------------------- //
// Interface implementation.
// ----------------------------------------------------------------------------- //

bool VoiceSE_Init()
{
	if (!snd_initialized)
		return false;

	g_SND_VoiceOverdriveInt = 256;
	return true;
}

void VoiceSE_Term()
{
	// Disable voice ducking.
	g_SND_VoiceOverdriveInt = 256;
}


void VoiceSE_Idle(float frametime)
{
	g_SND_VoiceOverdriveInt = 256;

	if (g_bVoiceOverdriveOn)
	{
		g_VoiceOverdriveDuration = MIN(g_VoiceOverdriveDuration+frametime, voice_overdrivefadetime.GetFloat());
	}
	else
	{
		if (g_VoiceOverdriveDuration == 0)
			return;

		g_VoiceOverdriveDuration = MAX(g_VoiceOverdriveDuration-frametime, 0);
	}

	float percent = g_VoiceOverdriveDuration / voice_overdrivefadetime.GetFloat();
	percent = (float)(-cos(percent * 3.1415926535) * 0.5 + 0.5); // Smooth it out..
	float voiceOverdrive = 1 + (voice_overdrive.GetFloat() - 1) * percent;
	g_SND_VoiceOverdriveInt = (int)(256 / voiceOverdrive);
}


int VoiceSE_StartChannel(
	int iChannel, //! Which channel to start.
	int iEntity,
	bool bProximity,
	int nViewEntityIndex)
{
	Assert(iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS);

	// Start the sound.
	CSfxTable* sfx = &g_CVoiceSfx[iChannel];
	sfx->pSource = nullptr;
	Vector vOrigin(0, 0, 0);

	StartSoundParams_t params;
	params.staticsound = false;
	params.entchannel = (CHAN_VOICE_BASE + iChannel);
	params.pSfx = sfx;
	params.origin = vOrigin;
	params.fvol = 1.0f;
	params.flags = 0;
	params.pitch = PITCH_NORM;


	if (bProximity == true)
	{
		params.bUpdatePositions = true;
		params.soundlevel = SNDLVL_TALKING;
		params.soundsource = iEntity;
	}
	else
	{
		params.soundlevel = SNDLVL_IDLE;
		params.soundsource = nViewEntityIndex;
	}


	return S_StartSound(params);
}

void VoiceSE_EndChannel(
	int iChannel, //! Which channel to stop.
	int iEntity
)
{
	Assert(iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS);

	S_StopSound(iEntity, CHAN_VOICE_BASE + iChannel);

	// Start the sound.
	CSfxTable* sfx = &g_CVoiceSfx[iChannel];
	sfx->pSource = nullptr;
}

void VoiceSE_StartOverdrive()
{
	g_bVoiceOverdriveOn = true;
}

void VoiceSE_EndOverdrive()
{
	g_bVoiceOverdriveOn = false;
}


void VoiceSE_InitMouth(int entnum)
{
}

void VoiceSE_CloseMouth(int entnum)
{
}

void VoiceSE_MoveMouth(int entnum, short* pSamples, int nSamples)
{
}


CAudioSource* Voice_SetupAudioSource(int soundsource, int entchannel)
{
	int iChannel = entchannel - CHAN_VOICE_BASE;
	if (iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS)
	{
		CSfxTable* sfx = &g_CVoiceSfx[iChannel];
		return new CAudioSourceVoice(sfx, iChannel);
	}
	else
		return nullptr;
}
