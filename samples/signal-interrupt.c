/* Cross-Platform System Programming Guide: L2: handle Ctrl+C signal from console */

#include <assert.h>
#include <stdio.h>

typedef void (*sig_handler_t)();

/** User's signal handler */
sig_handler_t sig_userhandler;

#ifdef _WIN32

#include <windows.h>

/** Called by OS when a CTRL event is received from console */
BOOL WINAPI sig_ctrl_handler(DWORD ctrl)
{
	if (ctrl == CTRL_C_EVENT) {
		sig_userhandler();
		return 1;
	}
	return 0;
}

int sig_int_subscribe(sig_handler_t handler)
{
	sig_userhandler = handler;
	SetConsoleCtrlHandler(sig_ctrl_handler, 1);
	return 0;
}

#else // UNIX:

#include <signal.h>

/** Called by OS when a signal we subscribed to is received */
void sig_handler(int signo)
{
	assert(signo == SIGINT);
	sig_userhandler();
}

int sig_int_subscribe(sig_handler_t handler)
{
	sig_userhandler = handler;

	struct sigaction sa = {};
	sa.sa_handler = sig_handler;
	return sigaction(SIGINT, &sa, NULL);
}

#endif

int quit;

void ctrlc_handler()
{
	quit = 1;
}

void main()
{
	// subscribe for receiving interrupt signals
	sig_int_subscribe(ctrlc_handler);

	int r = 0;
	while (!quit) {
		r++;
	}
	printf("Result: %d", r);
}
