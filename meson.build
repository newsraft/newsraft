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
	'src/feedeater.h',
	'src/feedeater.c',
	'src/debug.c',
	'src/path.c',
	'src/string.c',
	'src/db.c',
	'src/db-feeds.c',
	'src/db-items.c',
	'src/config.h',
	'src/config.c',
	'src/sets.c',
	'src/status.c',
	'src/input.c',
	'src/download.c',
	'src/items.c',
	'src/tags.c',
	'src/date-utils.c',
	'src/parse-format.c',
	'src/parse-format-generic.c',
	'src/parse-format-rss20.c',
	'src/parse-namespace.c',
	'src/parse-namespace-atom10.c',
	'src/parse-namespace-atom03.c',
	#'src/parse-namespace-mrss.c',
	'src/parse-namespace-rss10.c',
	#'src/parse-namespace-rss10content.c',
	'src/parse-namespace-rss10-dublincore.c',
	#'src/parse-namespace-rss10syndication.c',
	'src/contents.c',
	'src/html-entities.c',
	'src/html-plainifier.c',
]
incdir = include_directories('src')
executable(
	'feedeater',
	sources: feedeater_src,
	dependencies: feedeater_dep,
	include_directories: incdir
)
