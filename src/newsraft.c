#include <locale.h>
#include <unistd.h>
#include <curses.h>
#include <curl/curl.h>
#include "newsraft.h"

static inline void
print_usage(void)
{
	fputs("newsraft - feed reader for terminal\n"
	      "-f PATH  force use of PATH as feeds file\n"
	      "-c PATH  force use of PATH as config file\n"
	      "-d PATH  force use of PATH as database file\n"
	      "-l PATH  write log information to PATH\n"
	      "-v       print version and successfully exit\n"
	      "-h       print this message and successfully exit\n",
	      stderr);
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
			fputs(NEWSRAFT_VERSION "\n", stderr);
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

	if (register_signal_handlers()         == false) { error = 6;  goto undo1;  }
	if (assign_default_binds()             == false) { error = 7;  goto undo1;  }
	if (load_config()                      == false) { error = 8;  goto undo2;  }
	if (db_init()                          == false) { error = 9;  goto undo3;  }
	if (query_database_file_optimization() == false) { error = 10; goto undo4;  }
	if (create_global_section()            == false) { error = 11; goto undo4;  }
	if (parse_feeds_file()                 == false) { error = 12; goto undo5;  }
	if (curses_init()                      == false) { error = 13; goto undo5;  }
	if (adjust_list_menu()                 == false) { error = 14; goto undo6;  }
	if (create_format_buffers()            == false) { error = 15; goto undo7;  }
	if (status_recreate()                  == false) { error = 16; goto undo8;  }
	if (allocate_status_messages_buffer()  == false) { error = 17; goto undo9;  }
	if (counter_recreate()                 == false) { error = 18; goto undo9;  }
	if (initialize_update_threads()        == false) { error = 19; goto undo10; }
	if (curl_global_init(CURL_GLOBAL_DEFAULT)  != 0) { error = 20; goto undo11; }
	initialize_settings_of_list_menus();
	refresh_unread_items_count_of_all_sections();
	if (start_auto_updater_if_necessary()  == false) { error = 21; goto undo12; }

	enter_sections_menu_loop();

	finish_auto_updater_if_necessary();
	wait_for_all_threads_to_finish();

undo12:
	curl_global_cleanup();
undo11:
	terminate_update_threads();
undo10:
	counter_delete();
undo9:
	status_delete();
undo8:
	free_format_buffers();
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
	free_binds();
undo1:
	log_stop(error);
undo0:
	return error;
}
