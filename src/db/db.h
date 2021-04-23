#include <sqlite3.h>
extern sqlite3 *db;
int db_init(void);
void db_stop(void);
