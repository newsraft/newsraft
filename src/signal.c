#include <signal.h>
#include "newsraft.h"

static void
tell_program_to_terminate_safely_and_quickly(int dummy)
{
	(void)dummy;
	they_want_us_to_stop = true;
}

static void
tell_program_to_suspend(int dummy)
{
	(void)dummy;
	they_want_us_to_suspend = true;
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
	return true;
}

void
enable_suspend_signal_handler(void)
{
	struct sigaction old = {0};
	if (sigaction(SIGTSTP, NULL, &old) == 0 && old.sa_handler != SIG_IGN) {
		struct sigaction s = {0};
		s.sa_handler = &tell_program_to_suspend;
		s.sa_flags = SA_RESTART;
		sigaction(SIGTSTP, &s, NULL);
	}
}

void
disable_suspend_signal_handler(void)
{
	struct sigaction old = {0};
	if (sigaction(SIGTSTP, NULL, &old) == 0 && old.sa_handler != SIG_IGN) {
		struct sigaction s = {0};
		s.sa_handler = SIG_DFL;
		sigaction(SIGTSTP, &s, NULL);
	}
}

bool
is_suspend_signal_allowed(void)
{
	struct sigaction old = {0};
	return sigaction(SIGTSTP, NULL, &old) == 0 && old.sa_handler != SIG_IGN;
}

void
suspend_process(void)
{
	they_want_us_to_suspend = false;

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGTSTP);
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
	kill(0, SIGTSTP);
}
