/** Audio API Quick Start Guide: WASAPI: Enumerate devices
Link with -lole32 */
#define COBJMACROS
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <assert.h>
#include <stdio.h>

int playback = 1;

const GUID _CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e,0x3d, 0xc4,0x57,0x92,0x91,0x69,0x2e}};
const GUID _IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7,0x46, 0xde,0x8d,0xb6,0x36,0x17,0xe6}};
const PROPERTYKEY _PKEY_Device_FriendlyName = {{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

void main()
{
	CoInitializeEx(NULL, 0);

	// Create device enumerator object
	IMMDeviceEnumerator *enu = NULL;
	assert(0 == CoCreateInstance(&_CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &_IID_IMMDeviceEnumerator, (void**)&enu));

	u_int mode = (playback) ? eRender : eCapture;

	// Get array of devices from device enumerator
	IMMDeviceCollection *dcoll = NULL;
	assert(0 == IMMDeviceEnumerator_EnumAudioEndpoints(enu, mode, DEVICE_STATE_ACTIVE, &dcoll));

	// Get default device (optional)
	IMMDevice *defdev = NULL;
	assert(0 == IMMDeviceEnumerator_GetDefaultAudioEndpoint(enu, mode, eConsole, &defdev));
	IMMDevice_Release(defdev);

	// Enumerate all devices
	for (u_int i = 0;  ;  i++) {

		// Get device #i
		IMMDevice *dev = NULL;
		if (0 != IMMDeviceCollection_Item(dcoll, i, &dev))
			break;

		// Get properties array associated with this device
		IPropertyStore *props = NULL;
		assert(0 == IMMDevice_OpenPropertyStore(dev, STGM_READ, &props));

		// Get device name
		PROPVARIANT name;
		PropVariantInit(&name);
		assert(0 == IPropertyStore_GetValue(props, &_PKEY_Device_FriendlyName, &name));
		const wchar_t *device_name = name.pwszVal;

		// Get device ID
		wchar_t *device_id = NULL;
		assert(0 == IMMDevice_GetId(dev, &device_id));
		// We can use this 'device_id' to assign audio buffer to this specific device

		printf("Device #%u\n", i);

		// Release pointers
		PropVariantClear(&name);
		IPropertyStore_Release(props);
		IMMDevice_Release(dev);
		CoTaskMemFree(device_id);
	}

	IMMDeviceEnumerator_Release(enu);
	IMMDeviceCollection_Release(dcoll);
}
