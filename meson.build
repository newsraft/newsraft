project(
	'feedeater',
	'c',
	license: 'MIT',
	default_options: [
		'c_std=c18',
		'warning_level=3'
	]
)
add_project_arguments(
	[
		#'-O2',
		#'-D_FORTIFY_SOURCE=2',
		'-fanalyzer',
		'-fdiagnostics-color=always',
		'-DXML_NS',
		'-DXML_LARGE_SIZE',
		# most of internal expat integers (including unsigned ones)
		# are long long as long as XML_LARGE_SIZE defined
		'-DXML_FMT_INT_MOD="ll"',
		# undefine XML_UNICODE to make information be UTF-8 encoded
		'-UXML_UNICODE',
		# undefine XML_UNICODE_WCHAR_T to use char for characters
		'-UXML_UNICODE_WCHAR_T',
		'-DXML_FMT_STR="s"',
	],
	language : 'c'
)
feedeater_dep = [
	dependency('ncursesw'),
	dependency('libcurl'),
	dependency('expat'),
	dependency('sqlite3'),
]
feedeater_src = [
  'src/config.c',
  'src/config.h',
  'src/contents.c',
  'src/html_parser.c',
  'src/html_entities.c',
  'src/db.c',
  'src/debug.c',
  'src/download.c',
  'src/feedeater.c',
  'src/feedeater.h',
  'src/input.c',
  'src/items.c',
  'src/namespace.c',
  'src/namespaces/atom_10.c',
#  'src/namespaces/media_rss.c',
#  'src/namespaces/rss_10.c',
#  'src/namespaces/rss_10_content.c',
#  'src/namespaces/rss_10_dublin_core.c',
#  'src/namespaces/rss_10_syndication.c',
  'src/parse.c',
  'src/parse_generic.c',
  'src/parse_rss20.c',
  'src/path.c',
  'src/sets.c',
  'src/status.c',
  'src/string.c',
  'src/tags.c',
]
incdir = include_directories('src')
executable(
	'feedeater',
	sources: feedeater_src,
	dependencies: feedeater_dep,
	include_directories: incdir
)
