#include <signal.h>
#include "newsraft.h"

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
