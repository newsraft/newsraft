#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include "feedeater.h"

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int opt;
	while ((opt = getopt(argc, argv, "vhd:F:D:")) != -1) {
		switch (opt) {
			case 'F':
				if (set_feeds_path(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'D':
				if (set_db_path(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				if (debug_init(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				fprintf(stderr, "this is not even pre-alpha\n");
				exit(EXIT_SUCCESS);
				break;
			case 'h':
				fprintf(stderr,
				        "feedeater - feed reader for terminal\n"
				        "-F PATH  force use of PATH as feeds file\n"
				        "-D PATH  force use of PATH as database file\n"
				        "-d PATH  write debug information to PATH\n"
				        "-v       print version and exit\n"
				        "-h       print this message and exit\n");
				exit(EXIT_SUCCESS);
				break;
			default:
				fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

	int error = 0;
	if (db_init() != 0)           { error = 1; goto undo1; }
	if (load_sets() != 0)         { error = 2; goto undo2; }
	if (initscr() == NULL)        { error = 3; goto undo3; }
	if (status_create() != 0)     { error = 4; goto undo4; }
	if (input_create() != 0)      { error = 5; goto undo5; }
	run_sets_menu();
	input_delete();
undo5:
	status_delete();
undo4:
	endwin();
undo3:
	free_sets();
undo2:
	db_stop();
undo1:
	debug_stop();
	return error;
}
