#include "newsraft.h"

#define ERROR_BUFFER_SIZE 1000

static char error_buffer[ERROR_BUFFER_SIZE + 1];
static size_t error_buffer_len = 0;

// We have to write errors to intermediate buffer because when writing to the
// stderr directly, user interface proactively erase everything that was printed.

void
write_error(const char *format, ...)
{
	if (error_buffer_len < ERROR_BUFFER_SIZE) {
		va_list args;
		va_start(args, format);
		int add = vsnprintf(
			error_buffer + error_buffer_len,
			ERROR_BUFFER_SIZE - error_buffer_len,
			format,
			args
		);
		if (add > 0) {
			error_buffer_len += add;
		}
		va_end(args);
	}
}

void
flush_errors(void)
{
	if (error_buffer_len > 0) {
		fputs(error_buffer, stderr);
	}
}
