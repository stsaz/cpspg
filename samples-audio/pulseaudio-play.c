/** Audio API Quick Start Guide: PulseAudio: Play audio from stdin
Link with -lpulse */
#include <pulse/pulseaudio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

pa_threaded_mainloop *mloop;
int quit;

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

// Called within mainloop thread after I/O operation is complete
void on_io_complete(pa_stream *s, size_t nbytes, void *udata)
{
	pa_threaded_mainloop_signal(mloop, 0);
}

pa_stream* abuf_create(pa_context *ctx)
{
	// Create an audio buffer
	pa_stream *stm;
	pa_sample_spec spec;
	spec.format = PA_SAMPLE_S16LE;
	spec.rate = 48000;
	spec.channels = 2;
	assert(NULL != (stm = pa_stream_new(ctx, "My App", &spec, NULL)));

	fprintf(stderr, "Using format %u, sample rate %u, channels %u\n", spec.format, spec.rate, spec.channels);

	// Initialize device property-set with default values
	pa_buffer_attr attr;
	memset(&attr, 0xff, sizeof(attr));

	// Set the audio buffer size in bytes using buffer length in milliseconds (optional)
	int buffer_length_msec = 500;
	attr.tlength = spec.rate * 16/8 * spec.channels * buffer_length_msec / 1000;

	// Attach audio buffer to device
	void *udata = NULL;
	pa_stream_set_write_callback(stm, on_io_complete, udata);
	const char *device_id = NULL; // use default device
	pa_stream_connect_playback(stm, device_id, &attr, 0, NULL, NULL);

	// Wait until the attachment is complete
	for (;;) {
		int r = pa_stream_get_state(stm);
		if (r == PA_STREAM_READY) {
			break;
		} else if (r == PA_STREAM_FAILED) {
			assert(0);
		}

		pa_threaded_mainloop_wait(mloop);
	}

	return stm;
}

void on_sigint()
{
	quit = 1;
}

// Called within mainloop thread after operation is complete
void on_op_complete(pa_stream *s, int success, void *udata)
{
	pa_threaded_mainloop_signal(mloop, 0);
}

void main()
{
	pa_context *ctx = sv_connect();

	pa_threaded_mainloop_lock(mloop);

	pa_stream *stm = abuf_create(ctx);

	// Properly handle SIGINT from user
	struct sigaction sa = {};
	sa.sa_handler = on_sigint;
	sigaction(SIGINT, &sa, NULL);

	// Read audio samples from stdin and pass them to audio buffer
	while (!quit) {

		// Get the size of free space
		size_t n = pa_stream_writable_size(stm);
		if (n == 0) {
			// Buffer is full. Process more events.
			pa_threaded_mainloop_wait(mloop);
			continue;
		}

		// Get audio data region available for writing
		void *buf;
		assert(0 == pa_stream_begin_write(stm, &buf, &n));
		assert(buf != NULL);

		// Read data from stdin
		n = read(0, buf, n);

		// Mark the data chunk as complete
		assert(0 == pa_stream_write(stm, buf, n, NULL, 0, PA_SEEK_RELATIVE));

		if (n == 0)
			break; // stdin data is complete
	}

	// Wait until all bufferred data is played by audio device
	pa_operation *op;
	if (!quit) {
		void *udata = NULL;
		op = pa_stream_drain(stm, on_op_complete, udata);
	}
	while (!quit) {
		int r = pa_operation_get_state(op);
		if (r == PA_OPERATION_DONE || r == PA_OPERATION_CANCELLED) {
			pa_operation_unref(op);
			break;
		}
		pa_threaded_mainloop_wait(mloop);
	}

	pa_stream_disconnect(stm);
	pa_stream_unref(stm);
	pa_threaded_mainloop_unlock(mloop);

	sv_disconnect(ctx);
}
