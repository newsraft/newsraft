#include <signal.h>
#include "newsraft.h"

bool
register_signal_handlers(void)
{
	struct sigaction act = {0};
	act.sa_handler = &tell_program_to_terminate_safely_and_quickly;
	if (sigaction(SIGQUIT, &act, NULL) != 0) {
		fputs("Failed to register SIGQUIT signal handler!\n", stderr);
		return false;
	}
	if (sigaction(SIGINT, &act, NULL) != 0) {
		fputs("Failed to register SIGINT signal handler!\n", stderr);
		return false;
	}
	if (sigaction(SIGTERM, &act, NULL) != 0) {
		fputs("Failed to register SIGTERM signal handler!\n", stderr);
		return false;
	}
	return true;
}
