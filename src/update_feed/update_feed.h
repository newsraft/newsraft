#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include "feedeater.h"

#define NAMESPACE_SEPARATOR ' '

struct generator {
	struct string *name;
	struct string *version;
	struct string *url;
};

struct text {
	struct string *value;
	struct string *type;
};

struct person {
	struct string *name;
	struct string *email;
	struct string *link;
};

struct person_list {
	struct person *list; // Dynamic array of persons.
	size_t len;          // Shows how many items is in list.
	size_t lim;          // Shows how many items list can fit.
};

// Used to bufferize a feed before writing it to the database,
// so we can insert or replace one big chunk of data in one
// statement instead of frequent check-inserts of individual values.
struct feed_bucket {
	struct text title;
	struct string *link;
	struct text summary;
	struct string *categories;
	struct string *language;
	struct generator generator;
	struct text rights;
	time_t update_time;
	time_t download_time;
};

// Used to bufferize an item before writing it to the database,
// so we can ignore it in case if identical item is already cached.
struct item_bucket {
	struct string *guid;
	struct text title;
	struct string *url; // TODO: make this of struct link * type
	struct text summary;
	struct text content;
	struct string *categories;
	struct string *comments_url;
	struct link_list attachments;
	struct person_list authors;
	// Dates in this struct are represented in seconds
	// since the Epoch (1970-01-01 00:00 UTC). If some
	// date is set to 0 then it is considered unset.
	time_t pubdate;
	time_t upddate;
};

void delete_excess_items(const struct string *feed_url);

bool db_bind_text_struct(sqlite3_stmt *s, intmax_t placeholder, const struct text *text_struct);

struct string *generate_link_list_string_for_database(const struct link_list *links);
struct string *generate_person_list_string(const struct person_list *persons);
struct string *generate_generator_string_for_database(const struct generator *generator);

#endif // UPDATE_FEED_H
