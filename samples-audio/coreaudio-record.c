/** Audio API Quick Start Guide: CoreAudio: Record audio and pass to stdout
Link with -framework CoreFoundation -framework CoreAudio */
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "ringbuffer.h"

int quit;
ringbuffer *ring_buf;

const AudioObjectPropertyAddress prop_odev_default = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_idev_default = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_odev_fmt = { kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_idev_fmt = { kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeInput, kAudioObjectPropertyElementMaster };

OSStatus on_capture(AudioDeviceID device, const AudioTimeStamp *now,
	const AudioBufferList *indata, const AudioTimeStamp *intime,
	AudioBufferList *outdata, const AudioTimeStamp *outtime,
	void *udata)
{
	const float *d = indata->mBuffers[0].mData;
	size_t n = indata->mBuffers[0].mDataByteSize;

	ringbuf *ring = udata;
	ringbuf_write(ring, d, n);
	return 0;
}

void* abuf_create(int playback, void *proc, int *dev_id)
{
	AudioObjectID device_id;
	if (1) {
		// Get default device
		const AudioObjectPropertyAddress *a = (playback) ? &prop_odev_default : &prop_idev_default;
		u_int size = sizeof(AudioObjectID);
		assert(0 == AudioObjectGetPropertyData(kAudioObjectSystemObject, a, 0, NULL, &size, &device_id));
	}

	// Get audio format
	AudioStreamBasicDescription asbd = {};
	u_int size = sizeof(asbd);
	const AudioObjectPropertyAddress *a = (playback) ? &prop_odev_fmt : &prop_idev_fmt;
	assert(0 == AudioObjectGetPropertyData(device_id, a, 0, NULL, &size, &asbd));
	fprintf(stderr, "Using format float32, sample rate %u, channels %u\n", (int)asbd.mSampleRate, (int)asbd.mChannelsPerFrame);

	// Get buffer size
	int buffer_length_msec = 500;
	int buf_size = 32/8 * sample_rate * channels * buffer_length_msec / 1000;

	// Allocate buffer
	ring_buf = ringbuf_alloc(buf_size);

	// Register I/O callback
	void *io_proc_id = NULL;
	void *udata = ring_buf;
	assert(0 == AudioDeviceCreateIOProcID(device_id, proc, udata, (AudioDeviceIOProcID*)&io_proc_id)
		&& io_proc_id != NULL);

	*dev_id = device_id;
	return io_proc_id;
}

void on_sigint()
{
	quit = 1;
}

void main()
{
	int dev;
	void *io_proc_id = abuf_create(0, on_capture, &dev);

	// Properly handle SIGINT from user
	struct sigaction sa = {};
	sa.sa_handler = on_sigint;
	sigaction(SIGINT, &sa, NULL);

	// Start streaming
	assert(0 == AudioDeviceStart(dev, io_proc_id));

	while (!quit) {

		ringbuffer_chunk buf;
		size_t h = ringbuf_read_begin(ring_buf, -1, &buf, NULL);

		if (buf.len == 0) {
			// Buffer is empty. Wait 100ms until some new data is available
			int period_ms = 100;
			usleep(period_ms*1000);
			continue;
		}

		// Write to stdout
		write(1, buf.ptr, buf.len);

		ringbuf_read_finish(ring_buf, h);
	}

	AudioDeviceDestroyIOProcID(dev, io_proc_id);
	ringbuf_free(ring_buf);
}
