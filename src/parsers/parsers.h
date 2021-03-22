#include <libxml/tree.h>
int parse_rss20(xmlNodePtr node);  // RSS 2.0
int parse_rss11(xmlNodePtr node);  // RSS 1.1
int parse_rss10(xmlNodePtr node);  // RSS 1.0
int parse_rss094(xmlNodePtr node); // RSS 0.94
int parse_rss092(xmlNodePtr node); // RSS 0.92
int parse_rss091(xmlNodePtr node); // RSS 0.91
int parse_rss090(xmlNodePtr node); // RSS 0.90
int parse_atom10(xmlNodePtr node); // Atom 1.0
int parse_atom03(xmlNodePtr node); // Atom 0.3
int parse_json11(void);            // JSON 1.1
