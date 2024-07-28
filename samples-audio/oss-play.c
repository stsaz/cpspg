/** Audio API Quick Start Guide: OSS: Play audio from stdin
Link with -lm */
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int quit;

int abuf_create(int playback, void **data, int *buf_size, int *frame_size)
{
	// Open device
	int dsp;
	const char *device_id = NULL;
	if (device_id == NULL)
		device_id = "/dev/dsp";
	int flags = (playback) ? O_WRONLY : O_RDONLY;
	assert(0 < (dsp = open(device_id, flags | O_EXCL, 0)));

	// Set sample format
	int format = AFMT_S16_LE;
	assert(0 <= ioctl(dsp, SNDCTL_DSP_SETFMT, &format));

	// Set channels
	int channels = 2;
	assert(0 <= ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels));

	// Set sample rate
	int sample_rate = 44100;
	assert(0 <= ioctl(dsp, SNDCTL_DSP_SPEED, &sample_rate));

	fprintf(stderr, "Using format %u, sample rate %u, channels %u\n", format, sample_rate, channels);

	audio_buf_info info = {};

	// Set buffer length
	if (playback)
		assert(0 <= ioctl(dsp, SNDCTL_DSP_GETOSPACE, &info));
	else
		assert(0 <= ioctl(dsp, SNDCTL_DSP_GETISPACE, &info));

	int buffer_length_msec = 500;
	int frag_num = 16/8 * sample_rate * channels * buffer_length_msec / 1000 / info.fragsize;
	int fr = (frag_num << 16) | (int)log2(info.fragsize); // buf_size = frag_num * 2^n
	assert(0 <= ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &fr));

	// Get buffer length
	memset(&info, 0, sizeof(info));
	if (playback)
		assert(0 <= ioctl(dsp, SNDCTL_DSP_GETOSPACE, &info));
	else
		assert(0 <= ioctl(dsp, SNDCTL_DSP_GETISPACE, &info));
	buffer_length_msec = info.fragstotal * info.fragsize * 1000 / (16/8 * sample_rate * channels);
	*buf_size = info.fragstotal * info.fragsize;
	*frame_size = 16/8 * channels;

	// Create buffer for audio data
	*data = malloc(*buf_size);

	return dsp;
}

void on_sigint()
{
	quit = 1;
}

void main()
{
	void *buf;
	int buf_size, frame_size;
	int dsp = abuf_create(1, &buf, &buf_size, &frame_size);

	// Properly handle SIGINT from user
	struct sigaction sa = {};
	sa.sa_handler = on_sigint;
	sigaction(SIGINT, &sa, NULL);

	while (!quit) {
		// Read data from stdin
		int n = read(0, buf, buf_size);
		assert(n >= 0);
		if (n == 0)
			break; // stdin data is complete
		assert(n%frame_size == 0);

		// Write audio samples to device
		n = write(dsp, buf, n);
		assert(n >= 0);
	}

	// Wait until all bufferred data is played by audio device
	if (!quit) {
		assert(0 <= ioctl(dsp, SNDCTL_DSP_SYNC, 0));
	}

	free(buf);
	close(dsp);
}
