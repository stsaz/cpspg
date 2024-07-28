/** Audio API Quick Start Guide: ALSA: Enumerate devices
Link with -lalsa */
#include <alsa/asoundlib.h>
#include <assert.h>
#include <stdio.h>

int playback = 1;

void main()
{
	int icard = -1;
	for (;;) {

		// Get next sound card
		assert(0 == snd_card_next(&icard));
		if (icard == -1)
			break;

		// Open sound card handler
		char scard[32];
		snprintf(scard, sizeof(scard), "hw:%u", icard);
		snd_ctl_t *sctl = NULL;
		if (0 != snd_ctl_open(&sctl, scard, 0))
			continue;

		// Get sound card info
		snd_ctl_card_info_t *scinfo = NULL;
		assert(0 == snd_ctl_card_info_malloc(&scinfo));
		assert(0 == snd_ctl_card_info(sctl, scinfo));
		const char *soundcard_name = snd_ctl_card_info_get_name(scinfo);
		snd_ctl_card_info_free(scinfo);

		int idev = -1;
		for (;;) {

			// Get next device
			if (0 != snd_ctl_pcm_next_device(sctl, &idev)
				|| idev == -1)
				break;

			// Get device info
			snd_pcm_info_t *pcminfo;
			snd_pcm_info_alloca(&pcminfo);
			snd_pcm_info_set_device(pcminfo, idev);

			int mode = (playback) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
			snd_pcm_info_set_stream(pcminfo, mode);

			// Get device properties
			if (0 != snd_ctl_pcm_info(sctl, pcminfo))
				break;

			const char *device_name = snd_pcm_info_get_name(pcminfo);

			char device_id[64];
			snprintf(device_id, sizeof(device_id), "plughw:%u,%u", icard, idev);
			// We can use this 'device_id' to assign audio buffer to this specific device

			printf("Device: %s: %s: %s\n", soundcard_name, device_name, device_id);
		}

		snd_ctl_close(sctl);
	}
}
