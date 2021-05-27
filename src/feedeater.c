#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "feedeater.h"
#include <locale.h>
static void
print_help(void)
{
	fprintf(stderr, "help!\n");
}

static void
print_version(void)
{
	fprintf(stderr, "version!\n");
}

int
main (int argc, char **argv)
{
	setlocale(LC_ALL, "");

	// parse command-line arguments
	int opt;
	while ((opt = getopt(argc, argv, "vhd:")) != -1) {
		switch (opt) {
			case 'd':
				if (debug_init(optarg) != 0) {
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				print_version();
				exit(EXIT_SUCCESS);
				break;
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			default:
				print_help();
				exit(EXIT_FAILURE);
				break;
		}
	}

	int error = 0;
	if (set_conf_dir_path() != 0) { error = 1; goto undo1; }
	if (set_data_dir_path() != 0) { error = 2; goto undo2; }
	if (db_init() != 0)           { error = 3; goto undo3; }
	if (load_feed_list() != 0)    { error = 4; goto undo4; }
	if (initscr() == NULL)        { error = 5; goto undo5; }
	if (status_create() != 0)     { error = 6; goto undo6; }
	if (input_create() != 0)      { error = 7; goto undo7; }
	run_feeds_menu();
	input_delete();
undo7:
	status_delete();
undo6:
	endwin();
undo5:
	free_feed_list();
undo4:
	db_stop();
undo3:
	free_data_dir_path();
undo2:
	free_conf_dir_path();
undo1:
	debug_stop();
	return error;
}
