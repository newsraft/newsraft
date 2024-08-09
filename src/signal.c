#include <signal.h>
#include "newsraft.h"

static sigset_t thread_wake_up_signals;

static void
tell_program_to_terminate_safely_and_quickly(int dummy)
{
	(void)dummy;
	they_want_us_to_stop = true;
}

void
wait_a_second_for_wake_up_signal(void)
{
	struct timespec delay = {1, 0L};
	sigtimedwait(&thread_wake_up_signals, NULL, &delay);
}

bool
register_signal_handlers(void)
{
	struct sigaction s = {0};
	s.sa_handler = &tell_program_to_terminate_safely_and_quickly;
	if (sigaction(SIGQUIT, &s, NULL) || sigaction(SIGINT, &s, NULL) || sigaction(SIGTERM, &s, NULL)) {
		write_error("Failed to register signal handlers!\n");
		return false;
	}

	if (sigemptyset(&thread_wake_up_signals) != 0) {
		write_error("Failed to initialize thread wake up signals!\n");
		return false;
	}
	if (sigaddset(&thread_wake_up_signals, SIGUSR1) != 0) {
		write_error("Failed to populate thread wake up signals!\n");
		return false;
	}
	if (pthread_sigmask(SIG_BLOCK, &thread_wake_up_signals, NULL) != 0) {
		write_error("Failed to apply block mask to thread wake up signals!\n");
		return false;
	}

	return true;
}
