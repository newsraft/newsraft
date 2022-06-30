project(
	'newsraft',
	'c',
	default_options: [
		'c_std=gnu18',
		'warning_level=3',
		'buildtype=release',
	],
	license: 'MIT',
	version: '0.3',
)
if get_option('buildtype') == 'release'
	add_project_arguments(['-fdiagnostics-color=never'], language: 'c')
else
	add_project_arguments(['-fdiagnostics-color=auto'], language: 'c')
	# cppcheck analysis is so trivial, but sometimes it is worth checking
	# you can execute "ninja cppcheck" to run code analysis
	# run_target('cppcheck', command: ['cppcheck', '--project=' + join_paths(meson.build_root(), 'compile_commands.json')])
endif

newsraft_sources = [
	'src/newsraft.c',
	'src/log.c',
	'src/path.c',
	'src/string.c',
	'src/string-serialize.c',
	'src/wstring.c',
	'src/dates.c',
	'src/db.c',
	'src/db-items.c',
	'src/feeds.c',
	'src/feeds-parse.c',
	'src/feeds-verifier.c',
	'src/sections.c',
	'src/input.c',
	'src/input-macros.c',
	'src/input-binds-default.c',
	'src/render-block.c',
	'src/commands.c',
	'src/interface.c',
	'src/interface-colors.c',
	'src/interface-status.c',
	'src/interface-counter.c',
	'src/interface-list.c',
	'src/interface-list-format.c',
	'src/interface-pager.c',
	'src/interface-pager-item.c',
	'src/interface-pager-status.c',
	'src/items.c',
	'src/items-list.c',
	'src/items-links.c',
	'src/items-persons.c',
	'src/items-metadata.c',
	'src/items-metadata-content.c',
	'src/load_config/load_config.c',
	'src/load_config/config.c',
	'src/load_config/config-parse.c',
	'src/load_config/config-parse-input.c',
	'src/load_config/config-useragent.c',
	'src/load_config/config-openinbrowsercommand.c',
	'src/load_config/config-copytoclipboardcommand.c',
	'src/load_config/config-verifier.c',
	'src/update_feed/update_feed.c',
	'src/update_feed/download.c',
	'src/update_feed/struct-item.c',
	'src/update_feed/parse_xml/engage-xml-parser.c',
	'src/update_feed/parse_xml/xml-common.c',
	'src/update_feed/parse_json/engage-json-parser.c',
	'src/update_feed/insert_feed/insert_feed.c',
	'src/update_feed/insert_feed/insert-feed-data.c',
	'src/update_feed/insert_feed/insert-item-data.c',
	'src/prepare_to_render_data/prepare_to_render_data.c',
	'src/prepare_to_render_data/prepare-text-html.c',
	'src/render_data/render_data.c',
	'src/render_data/line.c',
	'src/render_data/render-text-plain.c',
	'src/render_data/render-text-html.c',
	'src/render_data/render-text-html-table.c',
]

if get_option('support-format-atom10').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_ATOM10'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-atom10.c']
endif
if get_option('support-format-rss').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_RSS'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-rss.c']
endif
if get_option('support-format-rsscontent').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_RSSCONTENT'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-rsscontent.c']
endif
if get_option('support-format-dublincore').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_DUBLINCORE'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-dublincore.c']
endif
if get_option('support-format-mediarss').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_MEDIARSS'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-mediarss.c']
endif
if get_option('support-format-yandex').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_YANDEX'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-yandex.c']
endif
if get_option('support-format-rbcnews').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_RBCNEWS'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-rbcnews.c']
endif
if get_option('support-format-atom03').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_ATOM03'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-atom03.c']
endif
if get_option('support-format-georss').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_GEORSS'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-georss.c']
endif
if get_option('support-format-georss-gml').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_GEORSS_GML'], language: 'c')
	newsraft_sources += ['src/update_feed/parse_xml/xml-handler-georss-gml.c']
endif
if get_option('support-format-jsonfeed').enabled()
	add_project_arguments(['-DNEWSRAFT_FORMAT_SUPPORT_JSONFEED'], language: 'c')
endif

add_project_arguments(['-DNEWSRAFT_VERSION="@0@"'.format(meson.project_version())], language: 'c')

executable(
	'newsraft',
	include_directories: [
		include_directories('src'),
	],
	sources: newsraft_sources,
	dependencies: [
		dependency('ncursesw'),
		dependency('libcurl'),
		dependency('sqlite3'),
		dependency('expat'),
		dependency('yajl'),
		dependency('gumbo'),
	],
	#link_args: ['-static'],
	install: true,
)

scdoc = dependency('scdoc', version: '>=1.11.2', native: true, required: get_option('man-page'))
if scdoc.found()
	scdoc_program = find_program('scdoc', native: true, required: get_option('man-page'))
	if scdoc_program.found()
		mandir = get_option('mandir')
		custom_target(
			'newsraft.1',
			input: 'doc/newsraft.scd',
			output: 'newsraft.1',
			command: scdoc_program,
			install: true,
			feed: true,
			capture: true,
			install_dir: '@0@/man1'.format(mandir)
		)
	endif
endif
