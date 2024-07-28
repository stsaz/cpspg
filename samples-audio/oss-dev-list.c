/** Audio API Quick Start Guide: OSS: Enumerate devices
Link with -lm */
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

int playback = 1;

void main()
{
	// Get sound system descriptor
	int mixer;
	assert(-1 != (mixer = open("/dev/mixer", O_RDONLY, 0)));

	// Get devices info
	oss_sysinfo si = {};
	assert(0 == ioctl(mixer, SNDCTL_SYSINFO, &si));
	u_int n_devs = si.numaudios;

	for (u_int i = 0;  i != n_devs;  i++) {

		// Get info for device #i
		oss_audioinfo ainfo = {};
		ainfo.dev = i;
		assert(0 == ioctl(mixer, SNDCTL_AUDIOINFO_EX, &ainfo));

		// Filter playback or capture devices
		if (!((playback && (ainfo.caps & PCM_CAP_OUTPUT))
			|| (!playback && (ainfo.caps & PCM_CAP_INPUT))))
			continue;

		const char *device_id = ainfo.devnode;
		const char *device_name = ainfo.name;
		printf("Device: %s\n", device_name);
	}

	close(mixer);
}
