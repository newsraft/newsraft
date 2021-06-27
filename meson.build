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
		'-fanalyzer',
		'-fdiagnostics-color=always',
		'-DXML_NS',
		'-DXML_LARGE_SIZE',
		'-DXML_FMT_INT_MOD="ll"',
		'-DXML_FMT_STR="s"',
		'-UXML_UNICODE',
		'-UXML_UNICODE_WCHAR_T',
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
