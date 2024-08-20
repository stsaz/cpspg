/* Cross-Platform System Programming Guide: L2: handle CPU exceptions */

#include <stdio.h>

struct sig_info {
	unsigned int sig; // enum CPSIG
	void *addr; // CPSIG_SEGV: the virtual address of the inaccessible data
	unsigned int flags; // Platform-specific flags related to the signal
};

typedef void (*sig_handler_t)(struct sig_info *i);

/** User's signal handler */
sig_handler_t _sig_userhandler;

#ifdef _WIN32

#include <windows.h>

enum CPSIG {
	CPSIG_SEGV = EXCEPTION_ACCESS_VIOLATION, // The thread tried to read from or write to a virtual address for which it does not have the appropriate access
	CPSIG_ILL = EXCEPTION_ILLEGAL_INSTRUCTION, // The thread tried to execute an invalid instruction
	CPSIG_STACK = EXCEPTION_STACK_OVERFLOW, // The thread used up its stack
	CPSIG_FPE = EXCEPTION_FLT_DIVIDE_BY_ZERO,
};

LONG WINAPI _sig_exc_handler(struct _EXCEPTION_POINTERS *inf)
{
	SetUnhandledExceptionFilter(NULL);

	struct sig_info i = {};
	i.sig = inf->ExceptionRecord->ExceptionCode; // EXCEPTION_*

	switch (inf->ExceptionRecord->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		i.flags = inf->ExceptionRecord->ExceptionInformation[0];
		i.addr = (void*)inf->ExceptionRecord->ExceptionInformation[1];
		break;
	}

	_sig_userhandler(&i);
	return EXCEPTION_CONTINUE_SEARCH;
}

int sig_subscribe(sig_handler_t handler, const unsigned int *sigs, unsigned int sigs_n)
{
	_sig_userhandler = handler;
	SetUnhandledExceptionFilter(_sig_exc_handler);
	return 0;
}

#else // UNIX:

#include <signal.h>
#include <stdlib.h>

enum CPSIG {
	CPSIG_SEGV = SIGSEGV, // Invalid memory reference
	CPSIG_FPE = SIGFPE, // Floating-point exception
	CPSIG_ILL = SIGILL, // Illegal Instruction
	CPSIG_STACK = 0x40000000 | SIGSEGV, // Stack overflow
};

void _sig_exc_handler(int signo, siginfo_t *info, void *ucontext)
{
	struct sig_info i = {};
	i.sig = (unsigned int)signo;
	i.addr = info->si_addr;
	i.flags = (unsigned int)info->si_code;
	_sig_userhandler(&i);
}

int sig_subscribe(sig_handler_t handler, const unsigned int *sigs, unsigned int sigs_n)
{
	_sig_userhandler = handler;

	int have_stack_sig = 0;
	struct sigaction sa = {};
	sa.sa_sigaction = _sig_exc_handler;

	for (unsigned int i = 0;  i != sigs_n;  i++) {
		if (sigs[i] == CPSIG_STACK) {
			stack_t stack;
			stack.ss_sp = malloc(SIGSTKSZ);
			if (stack.ss_sp == NULL)
				return -1;
			stack.ss_size = SIGSTKSZ;
			stack.ss_flags = 0;
			if (0 != sigaltstack(&stack, NULL))
				return -1;

			sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_ONSTACK;
			if (0 != sigaction(SIGSEGV, &sa, NULL))
				return -1;
			have_stack_sig = 1;
			break;
		}
	}

	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	for (unsigned int i = 0;  i != sigs_n;  i++) {
		if (sigs[i] == CPSIG_STACK
			|| (sigs[i] == SIGSEGV && have_stack_sig))
			continue;

		if (0 != sigaction(sigs[i], &sa, NULL))
			return -1;
	}
	return 0;
}

#endif

/** Raise the specified signal */
void sig_raise(int sig)
{
	switch (sig) {
	case CPSIG_STACK:
		sig_raise(sig);
		break;

	case CPSIG_SEGV: {
		void *addr = (void*)0x16;
		*(int*)addr = -1;
		break;
	}

	case CPSIG_FPE: {
		int i = 0;
		i = 10 / i;
		break;
	}
	}
}

void sig_handler(struct sig_info *i)
{
	printf("Signal:%x  Address:%p  Flags:%x\n"
		, i->sig, i->addr, i->flags);
}

void main(int argc, char **argv)
{
	unsigned int sigs[] = {
		CPSIG_SEGV, CPSIG_STACK, CPSIG_ILL, CPSIG_FPE
	};
	sig_subscribe(sig_handler, sigs, 4);

	if (argc > 1) {
		unsigned int i = argv[1][0] - '0';
		if (i < 4)
			sig_raise(sigs[i]);
	}
}
