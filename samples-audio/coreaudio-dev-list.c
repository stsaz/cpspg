/** Audio API Quick Start Guide: CoreAudio: Enumerate devices
Link with -framework CoreFoundation -framework CoreAudio */
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u_int;
int playback = 1;

const AudioObjectPropertyAddress prop_dev_list = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_dev_outconf = { kAudioDevicePropertyStreamConfiguration, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_dev_inconf = { kAudioDevicePropertyStreamConfiguration, kAudioDevicePropertyScopeInput, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_dev_outname = { kAudioObjectPropertyName, kAudioDevicePropertyScopeOutput, kAudioObjectPropertyElementMaster };
const AudioObjectPropertyAddress prop_dev_inname = { kAudioObjectPropertyName, kAudioDevicePropertyScopeInput, kAudioObjectPropertyElementMaster };

void main()
{
	// Determine the size needed for devices array
	u_int size;
	assert(kAudioHardwareNoError == AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &prop_dev_list, 0, NULL, &size));

	// Get devices array
	AudioObjectID *devs = (AudioObjectID*)malloc(size);
	assert(kAudioHardwareNoError == AudioObjectGetPropertyData(kAudioObjectSystemObject, &prop_dev_list, 0, NULL, &size, devs));

	u_int n_dev = size / sizeof(AudioObjectID);

	for (u_int i = 0;  i != n_dev;  i++) {

		const AudioObjectPropertyAddress *prop = (playback) ? &prop_dev_outconf : &prop_dev_inconf;
		assert(kAudioHardwareNoError == AudioObjectGetPropertyDataSize(devs[i], prop, 0, NULL, &size));

		AudioBufferList *bufs = (AudioBufferList*)malloc(size);
		assert(kAudioHardwareNoError == AudioObjectGetPropertyData(devs[i], prop, 0, NULL, &size, bufs));

		u_int ch = 0;
		for (u_int i = 0;  i != bufs->mNumberBuffers;  i++) {
			ch |= bufs->mBuffers[i].mNumberChannels;
		}
		free(bufs);
		if (ch == 0)
			continue;

		// Get device name
		prop = (playback) ? &prop_dev_outname : &prop_dev_inname;
		size = sizeof(CFStringRef);
		CFStringRef cfs = NULL;
		assert(kAudioHardwareNoError == AudioObjectGetPropertyData(devs[i], prop, 0, NULL, &size, &cfs));

		CFIndex len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfs), kCFStringEncodingUTF8);
		char *device_name = malloc(len + 1);
		assert(CFStringGetCString(cfs, device_name, len + 1, kCFStringEncodingUTF8));
		CFRelease(cfs);

		printf("Device: %s\n", device_name);
		free(device_name);
	}

	free(devs);
}
