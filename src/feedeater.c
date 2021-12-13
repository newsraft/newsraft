#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <curl/curl.h>
#include "feedeater.h"

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int opt;
	while ((opt = getopt(argc, argv, "vhf:c:d:l:")) != -1) {
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
			case 'l':
				if (log_init(optarg) != 0) {
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
				        "-l PATH  write log information to PATH\n"
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
	if (load_default_binds() != 0)                  { error = 1; goto undo1; }
	if (load_config() != 0)                         { error = 2; goto undo2; }
	if (db_init() != 0)                             { error = 3; goto undo3; }
	if (load_sets() != 0)                           { error = 4; goto undo4; }
	if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) { error = 5; goto undo5; }
	if (curses_init() != 0)                         { error = 6; goto undo6; }
	if (create_list_menu() != 0)                    { error = 7; goto undo7; }
	if (status_create() != 0)                       { error = 8; goto undo8; }
	if (reallocate_format_buffer() != 0)            { error = 9; goto undo9; }

	enter_sets_menu_loop();

	free_format_buffer();
undo9:
	status_delete();
undo8:
	free_list_menu();
undo7:
	endwin();
undo6:
	curl_global_cleanup();
undo5:
	free_sets();
undo4:
	db_stop();
undo3:
	free_config_data();
undo2:
	free_binds();
undo1:
	log_stop();
	return error;
}
