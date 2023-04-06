//
// NOTE: Source has a bunch of windows SDK headers checked in to the dx9sdk directory.
// This file is here to isolate dependencies on the newer version of the windows SDK
// NOTE: This code only applies to VISTA and greater

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#define SAFE_RELEASE(punk)  \
	if ((punk) != NULL)  \
		{ (punk)->Release(); (punk) = NULL; }

class CMMNotificationClient : public IMMNotificationClient
{
	LONG _cRef;
	IMMDeviceEnumerator *_pEnumerator;


public:
	CMMNotificationClient() :
	  _cRef(1),
		  _pEnumerator(nullptr)
	  {
	  }

	  ~CMMNotificationClient()
	  {
		  SAFE_RELEASE(_pEnumerator)
	  }

	  // IUnknown methods -- AddRef, Release, and QueryInterface

	  ULONG STDMETHODCALLTYPE AddRef()
	  {
		  return InterlockedIncrement(&_cRef);
	  }

	  ULONG STDMETHODCALLTYPE Release()
	  {
		  ULONG ulRef = InterlockedDecrement(&_cRef);
		  if (0 == ulRef)
		  {
			  delete this;
		  }
		  return ulRef;
	  }

	  HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, VOID **ppvInterface)
	  {
		  if (IID_IUnknown == riid)
		  {
			  AddRef();
			  *ppvInterface = (IUnknown*)this;
		  }
		  else if (__uuidof(IMMNotificationClient) == riid)
		  {
			  AddRef();
			  *ppvInterface = (IMMNotificationClient*)this;
		  }
		  else
		  {
			  *ppvInterface = nullptr;
			  return E_NOINTERFACE;
		  }
		  return S_OK;
	  }

	  // Callback methods for device-event notifications.

	  HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged( EDataFlow flow, ERole /*role*/, LPCWSTR /*pwstrDeviceId*/ )
	  {
		  return S_OK;
	  }

	  HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR /*pwstrDeviceId*/) { return S_OK; };
	  HRESULT STDMETHODCALLTYPE OnDeviceRemoved( LPCWSTR /*pwstrDeviceId*/ ) { return S_OK; }
	  HRESULT STDMETHODCALLTYPE OnDeviceStateChanged( LPCWSTR /*pwstrDeviceId*/, DWORD /*dwNewState*/ ) { return S_OK; }
	  HRESULT STDMETHODCALLTYPE OnPropertyValueChanged( LPCWSTR /*pwstrDeviceId*/, const PROPERTYKEY /*key*/ ) { return S_OK; }
};


extern CMMNotificationClient *g_pNotify;

HRESULT SetupWindowsMixerPreferences( float flMasterVolume = 1.0f, bool bDuckingOptOut = true )
{
	HRESULT hr = S_OK;

	IMMDeviceEnumerator* pDeviceEnumerator = nullptr;
	IMMDevice* pEndpoint = nullptr;
	IAudioSessionManager2* pSessionManager2 = nullptr;
	IAudioSessionControl* pSessionControl = nullptr;
	IAudioSessionControl2* pSessionControl2 = nullptr;


	//  Start with the default endpoint.

	hr = CoCreateInstance( __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDeviceEnumerator) );

	if ( SUCCEEDED( hr ) )
	{
		hr = pDeviceEnumerator->GetDefaultAudioEndpoint( eRender, eConsole, &pEndpoint);
		g_pNotify = new CMMNotificationClient;
		pDeviceEnumerator->RegisterEndpointNotificationCallback( g_pNotify );
		pDeviceEnumerator->Release();
		pDeviceEnumerator = nullptr;
	}

	// Activate session manager.
	if (SUCCEEDED(hr))
	{
		hr = pEndpoint->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>(&pSessionManager2) );
		if ( !SUCCEEDED( hr ) )
		{
			// probably on vista, get the regular session manager
			IAudioSessionManager *pSessionManager = nullptr;
			HRESULT hrVista = pEndpoint->Activate(__uuidof(IAudioSessionManager), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>(&pSessionManager) );
			if ( SUCCEEDED( hrVista ) )
			{
				ISimpleAudioVolume *pSimpleVolume = nullptr;
				HRESULT hrVolume = pSessionManager->GetSimpleAudioVolume(nullptr, 0, &pSimpleVolume );
				if ( SUCCEEDED( hrVolume ) )
				{
					pSimpleVolume->SetMasterVolume( flMasterVolume, nullptr);
					pSimpleVolume->Release();
				}
				pSessionManager->Release();
				pSessionManager = nullptr;
			}
		}
		pEndpoint->Release();
		pEndpoint = nullptr;
	}

	if ( SUCCEEDED( hr ) )
	{
		hr = pSessionManager2->GetAudioSessionControl(nullptr, 0, &pSessionControl);
		if ( SUCCEEDED( hr ) )
		{
			ISimpleAudioVolume *pSimpleVolume = nullptr;
			HRESULT hrVolume = pSessionManager2->GetSimpleAudioVolume(nullptr, FALSE, &pSimpleVolume );
			if ( SUCCEEDED( hrVolume ) )
			{
				pSimpleVolume->SetMasterVolume( flMasterVolume, nullptr);
				pSimpleVolume->Release();
			}
		}

		pSessionManager2->Release();
		pSessionManager2 = nullptr;
	}

	if ( SUCCEEDED( hr ) )
	{
		hr = pSessionControl->QueryInterface( __uuidof(IAudioSessionControl2), (void**)&pSessionControl2 );

		pSessionControl->Release();
		pSessionControl = nullptr;
	}

	//  Sync the ducking state with the specified preference.

	if ( SUCCEEDED( hr ) )
	{
		if (bDuckingOptOut)
		{
			hr = pSessionControl2->SetDuckingPreference(TRUE);
		}
		else
		{
			hr = pSessionControl2->SetDuckingPreference(FALSE);
		}
		pSessionControl2->Release();
		pSessionControl2 = nullptr;
	}
	return hr;
}

