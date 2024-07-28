/** Audio API Quick Start Guide: WASAPI: Play audio from stdin
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
const GUID _IID_IAudioRenderClient = {0xf294acfc, 0x3146, 0x4483, {0xa7,0xbf, 0xad,0xdc,0xa7,0xc2,0x60,0xe2}};

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
	printf("Using format float32, sample rate %u, channels %u\n", (int)wf->nSamplesPerSec, (int)wf->nChannels);

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
	IAudioClient *client = abuf_create(1, 0, &buf_frames, &frame_size);

	// Get interface for an audio capture object
	IAudioRenderClient *render;
	assert(0 == IAudioClient_GetService(client, &_IID_IAudioRenderClient, (void**)&render));

	// Properly handle Ctrl+C
	SetConsoleCtrlHandler(on_ctrl, 1);

	// Read audio samples from stdin and pass them to audio buffer
	int started = 0;
	while (!quit) {

		// Get the amount of free space
		u_int filled;
		assert(0 == IAudioClient_GetCurrentPadding(client, &filled));
		int n_free_frames = buf_frames - filled;

		if (n_free_frames == 0) {
			if (!started) {
				// Start playback
				assert(0 == IAudioClient_Start(client));
				started = 1;
			}

			// Buffer is full. Wait.
			int period_ms = 100;
			Sleep(period_ms);
			continue;
		}

		// Get free buffer region
		u_char *data;
		assert(0 == IAudioRenderClient_GetBuffer(render, n_free_frames, &data));

		// Read from stdin
		int n = n_free_frames * frame_size;
		u_long read;
		ReadFile(GetStdHandle(STD_INPUT_HANDLE), data, n, &read, 0);
		assert(read%frame_size == 0);

		// Release the buffer
		assert(0 == IAudioRenderClient_ReleaseBuffer(render, n_free_frames, 0));

		if (read == 0)
			break; // stdin data is complete
	}

	// Wait until all bufferred data is played by audio device
	while (!quit) {

		u_int filled;
		assert(0 == IAudioClient_GetCurrentPadding(client, &filled));

		if (filled == 0)
			break;

		if (!started) {
			// Start playback
			assert(0 == IAudioClient_Start(client));
			started = 1;
		}

		// Buffer isn't empty. Wait.
		int period_ms = 100;
		Sleep(period_ms);
	}

	IAudioRenderClient_Release(render);
	IAudioClient_Release(client);
}
