#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>

#define NAMESPACE_SEPARATOR ' '

enum xml_pos {
	IN_ROOT = 0,
	IN_ITEM_ELEMENT = 1,
	IN_TITLE_ELEMENT = 2,
	IN_DESCRIPTION_ELEMENT = 4,
	IN_LINK_ELEMENT = 8,
	IN_PUBDATE_ELEMENT = 16,
	IN_GUID_ELEMENT = 32,
	IN_AUTHOR_ELEMENT = 64,
	IN_CATEGORY_ELEMENT = 128,
	IN_COMMENTS_ELEMENT = 256,
	IN_ENCLOSURE_ELEMENT = 512,
	IN_SOURCE_ELEMENT = 1024,
	IN_IMAGE_ELEMENT = 2048,
	IN_UPDDATE_ELEMENT = 4096,
	IN_CHANNEL_ELEMENT = 8192,
	IN_NAME_ELEMENT = 16384,
	IN_URL_ELEMENT = 32768,
	IN_EMAIL_ELEMENT = 65536,
};

enum parse_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
	PARSE_FAIL_CURL_EASY_PERFORM_ERROR,
	PARSE_FAIL_XML_PARSE_ERROR,
};

struct link {
	struct string *url; // string with the url to data
	struct string *type; // standard MIME type of data
	size_t size; // size of data in bytes
	size_t duration; // duration of data (if it is a audio or video)
};

struct author {
	struct string *name;
	struct string *email;
	struct string *link;
};

// used to bufferize item before writing to disk
// so we can reject it in case parsed item is already cached
struct item_bucket {
	struct string *guid;
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *comments;
	struct string *categories;
	struct link *enclosures;
	size_t enclosures_count;
	struct author *authors;
	size_t authors_count;
	time_t pubdate;
	time_t upddate;
};

struct parser_data {
	char *value;
	size_t value_len;
	size_t value_lim;
	int depth;
	int pos;
	const struct string *feed_url;
	struct item_bucket *bucket;
	XML_Parser parser;
	void (*start_handler)(struct parser_data *data, const XML_Char *name, const XML_Char **atts);
	void (*end_handler)(struct parser_data *data, const XML_Char *name);
	enum parse_error error;
};

void db_update_feed_text(const struct string *feed_url, const char *column, const char *data, size_t data_len);
const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);
void try_item_bucket(const struct item_bucket *bucket, const struct string *feed_url);

// item bucket functions
struct item_bucket *create_item_bucket(void);
void drop_item_bucket(struct item_bucket *bucket);
void free_item_bucket(struct item_bucket *bucket);
void add_category_to_item_bucket(const struct item_bucket *bucket, const char *category, size_t category_len);
int add_enclosure_to_item_bucket(struct item_bucket *bucket, const char *url, const char *type, int size, int duration);

// xml element handlers
int parse_namespace_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts);
int parse_namespace_element_end  (struct parser_data *data, const XML_Char *name);
void parse_rss20_element_start   (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss20_element_end     (struct parser_data *data, const XML_Char *name);
void parse_rss10_element_start   (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss10_element_end     (struct parser_data *data, const XML_Char *name);
void parse_atom10_element_start  (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_atom10_element_end    (struct parser_data *data, const XML_Char *name);
void parse_atom03_element_start  (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_atom03_element_end    (struct parser_data *data, const XML_Char *name);
void parse_dc_element_start      (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_dc_element_end        (struct parser_data *data, const XML_Char *name);

extern int64_t rss20_pos;
#endif // UPDATE_FEED_H
