#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include "feedeater.h"

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int opt;
	while ((opt = getopt(argc, argv, "vhf:c:d:D:")) != -1) {
		switch (opt) {
			case 'f':
				if (set_feeds_path(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				if (set_config_path(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				if (set_db_path(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'D':
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
				        "-f PATH  force use of PATH as feeds file\n"
				        "-c PATH  force use of PATH as config file\n"
				        "-d PATH  force use of PATH as database file\n"
				        "-D PATH  write debug information to PATH\n"
				        "-v       print version and successfully exit\n"
				        "-h       print this message and successfully exit\n");
				exit(EXIT_SUCCESS);
				break;
			default:
				fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

	int error = 0;
	if (load_config() != 0)      { error = 1; goto main_undo1; }
	if (db_init() != 0)          { error = 2; goto main_undo2; }
	if (load_sets() != 0)        { error = 3; goto main_undo3; }
	if (initscr() == NULL)       { error = 4; goto main_undo4; }
	if (create_list_menu() != 0) { error = 5; goto main_undo5; }
	if (status_create() != 0)    { error = 6; goto main_undo6; }
	if (input_create() != 0)     { error = 7; goto main_undo7; }

	enter_sets_menu_loop();

	input_delete();
main_undo7:
	status_delete();
main_undo6:
	free_list_menu();
main_undo5:
	endwin();
main_undo4:
	free_sets();
main_undo3:
	db_stop();
main_undo2:
	free_config_data();
main_undo1:
	debug_stop();
	return error;
}
