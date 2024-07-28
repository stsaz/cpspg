/** Audio API Quick Start Guide: PulseAudio: Enumerate devices
Link with -lpulse */
#include <pulse/pulseaudio.h>
#include <assert.h>
#include <stdio.h>

int playback = 1;
pa_threaded_mainloop *mloop;

// Called within mainloop thread after connection state with PA server changes
void on_state_change(pa_context *c, void *userdata)
{
	// Wake the main thread
	pa_threaded_mainloop_signal(mloop, 0);
}

pa_context* sv_connect()
{
	// Create a new thread for handling client-server operations - "mainloop" thread
	assert(NULL != (mloop = pa_threaded_mainloop_new()));

	// Create a connection context
	pa_mainloop_api *mlapi = pa_threaded_mainloop_get_api(mloop);
	pa_context *ctx;
	assert(NULL != (ctx = pa_context_new_with_proplist(mlapi, "My App", NULL)));

	// Set "on-connect" handler and begin connection
	assert(0 == pa_context_connect(ctx, NULL, 0, NULL));
	void *udata = NULL;
	pa_context_set_state_callback(ctx, on_state_change, udata);

	// Start mainloop thread
	assert(0 == pa_threaded_mainloop_start(mloop));

	pa_threaded_mainloop_lock(mloop); // Perform all operations with the mainloop locked
	// Wait until the connection is complete
	for (;;) {
		// Check the current state of the ongoing connection
		int r = pa_context_get_state(ctx);
		if (r == PA_CONTEXT_READY) {
			break;
		} else if (r == PA_CONTEXT_FAILED || r == PA_CONTEXT_TERMINATED) {
			assert(0);
		}

		// Not yet connected. Block execution until some signal arrives.
		pa_threaded_mainloop_wait(mloop);
	}
	pa_threaded_mainloop_unlock(mloop);

	return ctx;
}

void sv_disconnect(pa_context *ctx)
{
	pa_threaded_mainloop_lock(mloop);
	pa_context_disconnect(ctx);
	pa_context_unref(ctx);
	pa_threaded_mainloop_unlock(mloop);

	pa_threaded_mainloop_stop(mloop);
	pa_threaded_mainloop_free(mloop);
}

// Called within mainloop thread with a new playback device info or when there are no more devices
void on_dev_sink(pa_context *c, const pa_sink_info *info, int eol, void *udata)
{
	if (eol > 0) {
		// Listing is finished
		pa_threaded_mainloop_signal(mloop, 0);
		return;
	} else if (eol < 0) {
		assert(0);
	}

	const char *device_id = info->name;
	// We can use this 'device_id' to assign audio buffer to this specific device

	printf("Device: %s: %s\n", info->name, info->description);
}

// Called within mainloop thread with a new capture device info
void on_dev_source(pa_context *c, const pa_source_info *info, int eol, void *udata)
{
	if (eol > 0) {
		// Listing is finished
		pa_threaded_mainloop_signal(mloop, 0);
		return;
	} else if (eol < 0) {
		assert(0);
	}

	const char *device_id = info->name;
	// We can use this 'device_id' to assign audio buffer to this specific device

	printf("Device: %s: %s\n", info->name, info->description);
}

void main()
{
	pa_context *ctx = sv_connect();

	// Start an operation to enumerate all devices
	pa_threaded_mainloop_lock(mloop);
	pa_operation *op;
	void *udata = NULL;
	if (playback)
		op = pa_context_get_sink_info_list(ctx, on_dev_sink, udata);
	else
		op = pa_context_get_source_info_list(ctx, on_dev_source, udata);

	for (;;) {
		int r = pa_operation_get_state(op);
		if (r == PA_OPERATION_DONE || r == PA_OPERATION_CANCELLED)
			break;
		pa_threaded_mainloop_wait(mloop);
	}

	pa_operation_unref(op);
	pa_threaded_mainloop_unlock(mloop);

	sv_disconnect(ctx);
}
