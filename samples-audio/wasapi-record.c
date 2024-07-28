/** Audio API Quick Start Guide: WASAPI: Record audio and pass to stdout
Link with -lole32 */
#define COBJMACROS
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <assert.h>
#include <stdio.h>

int quit;

const GUID _CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e}};
const GUID _IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6}};
const GUID _IID_IAudioClient = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2}};
const GUID _IID_IAudioCaptureClient = {0xc8adbd64, 0xe71e, 0x48a0, {0xa4,0xde, 0x18,0x5c,0x39,0x5c,0xd3,0x17}};

IAudioClient* abuf_create(int playback, int loopback_mode, u_int *buf_frames, int *frame_size)
{
	// Create device enumerator object
	IMMDeviceEnumerator *enu = NULL;
	assert(0 == CoCreateInstance(&_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &_IID_IMMDeviceEnumerator, (void**)&enu));

	// Get specific or default device handler
	IMMDevice *dev = NULL;
	wchar_t *device_id = NULL;
	if (device_id == NULL) {
		int mode = (playback) ? eRender : eCapture;
		assert(0 == IMMDeviceEnumerator_GetDefaultAudioEndpoint(enu, mode, eConsole, &dev));
	} else {
		assert(0 == IMMDeviceEnumerator_GetDevice(enu, device_id, &dev));
	}

	// Create audio buffer
	IAudioClient *client = NULL;
	assert(0 == IMMDevice_Activate(dev, &_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&client));

	// Get audio format for WASAPI shared mode
	WAVEFORMATEX *wf = NULL;
	assert(0 == IAudioClient_GetMixFormat(client, &wf));
	fprintf(stderr, "Using format float32, sample rate %u, channels %u\n", (int)wf->nSamplesPerSec, (int)wf->nChannels);

	// Set buffer parameters
	int buffer_length_msec = 500;
	REFERENCE_TIME dur = buffer_length_msec * 1000 * 10;
	int mode = AUDCLNT_SHAREMODE_SHARED;
	int aflags = (loopback_mode) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
	assert(0 == IAudioClient_Initialize(client, mode, aflags, dur, dur, (void*)wf, NULL));

	// Get the actual buffer length
	assert(0 == IAudioClient_GetBufferSize(client, buf_frames));
	buffer_length_msec = *buf_frames * 1000 / wf->nSamplesPerSec;

	*frame_size = wf->nBlockAlign;

	CoTaskMemFree(wf);
	IMMDevice_Release(dev);
	IMMDeviceEnumerator_Release(enu);
	return client;
}

/** Called by OS when a CTRL event is received from console */
static BOOL WINAPI on_ctrl(DWORD ctrl)
{
	if (ctrl == CTRL_C_EVENT) {
		quit = 1;
		return 1;
	}
	return 0;
}

void main()
{
	CoInitializeEx(NULL, 0);

	u_int buf_frames;
	int frame_size;
	IAudioClient *client = abuf_create(0, 0, &buf_frames, &frame_size);

	// Get interface for an audio capture object
	IAudioCaptureClient *capt;
	assert(0 == IAudioClient_GetService(client, &_IID_IAudioCaptureClient, (void**)&capt));

	// Properly handle Ctrl+C
	SetConsoleCtrlHandler(on_ctrl, 1);

	// Start recording
	assert(0 == IAudioClient_Start(client));

	while (!quit) {

		// Get available audio samples
		u_char *data;
		u_int nframes;
		u_long flags;
		int r = IAudioCaptureClient_GetBuffer(capt, &data, &nframes, &flags, NULL, NULL);

		if (r == AUDCLNT_S_BUFFER_EMPTY) {
			// Buffer is empty. Wait for more data.
			int period_ms = 100;
			Sleep(period_ms);
			continue;
		}
		assert(r == 0);

		// Write to stdout
		int n = nframes * frame_size;
		u_long written;
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), data, n, &written, 0);

		// Release the buffer
		assert(0 == IAudioCaptureClient_ReleaseBuffer(capt, nframes));
	}

	IAudioCaptureClient_Release(capt);
	IAudioClient_Release(client);
}
