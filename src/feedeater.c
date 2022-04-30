#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <curl/curl.h>
#include "feedeater.h"

static inline void
print_usage(void)
{
	fprintf(stderr,
	        "feedeater - feed reader for terminal\n"
	        "-f PATH  force use of PATH as feeds file\n"
	        "-c PATH  force use of PATH as config file\n"
	        "-d PATH  force use of PATH as database file\n"
	        "-l PATH  write log information to PATH\n"
	        "-v       print version and successfully exit\n"
	        "-h       print this message and successfully exit\n");
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int error = 0;
	int opt;
	while ((opt = getopt(argc, argv, "f:c:d:vhl:")) != -1) {
		if (opt == 'f') {
			if (set_feeds_path(optarg) == false) {
				error = 2;
				goto undo1;
			}
		} else if (opt == 'c') {
			if (set_config_path(optarg) == false) {
				error = 3;
				goto undo1;
			}
		} else if (opt == 'd') {
			if (set_db_path(optarg) == false) {
				error = 4;
				goto undo1;
			}
		} else if (opt == 'h') {
			print_usage();
			goto undo1;
		} else if (opt == 'v') {
			fprintf(stderr, FEEDEATER_VERSION "\n");
			goto undo1;
		} else if (opt == 'l') {
			if (log_init(optarg) == false) {
				error = 5;
				goto undo0;
			}
		} else {
			fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
			error = 1;
			goto undo1;
		}
	}

	if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) { error = 6;  goto undo1; }
	if (load_config()                     == false) { error = 7;  goto undo2; }
	if (db_init()                         == false) { error = 8;  goto undo3; }
	if (load_feeds()                      == false) { error = 9;  goto undo4; }
	if (curses_init()                     == false) { error = 10; goto undo5; }
	if (adjust_list_menu()                == false) { error = 11; goto undo6; }
	if (adjust_list_menu_format_buffer()  == false) { error = 12; goto undo7; }
	if (status_create()                   == false) { error = 13; goto undo8; }

	enter_feeds_menu_loop();

	status_delete();
undo8:
	free_list_menu_format_buffer();
undo7:
	free_list_menu();
undo6:
	endwin();
undo5:
	free_sections();
undo4:
	db_stop();
undo3:
	free_config();
undo2:
	curl_global_cleanup();
undo1:
	log_stop();
undo0:
	free_feeds_path();
	free_config_path();
	free_db_path();
	return error;
}
