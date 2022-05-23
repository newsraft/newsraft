project(
	'feedeater',
	'c',
	default_options: [
		'c_std=gnu18',
		'warning_level=3',
		'buildtype=release',
	],
	license: 'MIT',
	version: 'this is not even pre-alpha',
)
add_project_arguments(
	[
		# for debugging
		#'-fanalyzer', # gcc
		#'-Xanalyzer', # clang
		# support for various formats
		'-DFEEDEATER_FORMAT_SUPPORT_ATOM10',       # Atom 1.0
		'-DFEEDEATER_FORMAT_SUPPORT_RSS20',        # RSS 0.91, 0.92, 0.93, 0.94, 2.0
		'-DFEEDEATER_FORMAT_SUPPORT_RSSCONTENT',
		'-DFEEDEATER_FORMAT_SUPPORT_DUBLINCORE',
		'-DFEEDEATER_FORMAT_SUPPORT_MEDIARSS',
		'-DFEEDEATER_FORMAT_SUPPORT_YANDEX',
		'-DFEEDEATER_FORMAT_SUPPORT_RSS11',        # RSS 0.9, 1.0, 1.1
		'-DFEEDEATER_FORMAT_SUPPORT_ATOM03',       # Atom 0.3
		'-DFEEDEATER_FORMAT_SUPPORT_JSONFEED',
	],
	language: 'c',
)
if get_option('buildtype') == 'release'
	add_project_arguments(
		[
			'-fdiagnostics-color=never',
		],
		language: 'c',
	)
else
	add_project_arguments(
		[
			'-fdiagnostics-color=always',
			#'-Og', # meson takes care of it
			#'-Qunused-arguments', # for clang
		],
		language: 'c',
	)
	# run "ninja cppcheck" to run code analysis
	run_target('cppcheck', command: ['cppcheck', '--project=' + join_paths(meson.build_root(), 'compile_commands.json')])
endif

scdoc = dependency('scdoc', version: '>=1.11.2', native: true, required: get_option('man-page'))
if scdoc.found()
	scdoc_program = find_program('scdoc', native: true, required: get_option('man-page'))
	if scdoc_program.found()
		mandir = get_option('mandir')
		custom_target(
			'feedeater.1',
			input: 'doc/feedeater.scd',
			output: 'feedeater.1',
			command: scdoc_program,
			install: true,
			feed: true,
			capture: true,
			install_dir: '@0@/man1'.format(mandir)
		)
	endif
endif

executable(
	'feedeater',
	include_directories: [
		include_directories('src'),
	],
	sources: [
		'src/feedeater.c',
		'src/log.c',
		'src/path.c',
		'src/string.c',
		'src/wstring.c',
		'src/dates.c',
		'src/db.c',
		'src/db-items.c',
		'src/feeds.c',
		'src/feeds-verifier.c',
		'src/feeds-sections.c',
		'src/input.c',
		'src/interface.c',
		'src/interface-status.c',
		'src/interface-list.c',
		'src/interface-list-format.c',
		'src/interface-pager.c',
		'src/interface-pager-item.c',
		'src/interface-pager-status.c',
		'src/items.c',
		'src/items-list.c',
		'src/items-links.c',
		'src/items-content.c',
		'src/items-metadata.c',
		'src/load_config/load_config.c',
		'src/load_config/config.c',
		'src/load_config/config-binds-default.c',
		'src/load_config/config-useragent.c',
		'src/load_config/config-verifier.c',
		'src/update_feed/update_feed.c',
		'src/update_feed/struct-feed.c',
		'src/update_feed/struct-item.c',
		'src/update_feed/struct-link.c',
		'src/update_feed/struct-person.c',
		'src/update_feed/struct-category.c',
		'src/update_feed/struct-generator.c',
		'src/update_feed/download_feed/download_feed.c',
		'src/update_feed/download_feed/create-headers-list.c',
		'src/update_feed/parse_feed/parse_feed.c',
		'src/update_feed/parse_feed/date-utils.c',
		'src/update_feed/parse_feed/parse_xml_feed/parse_xml_feed.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-common.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-dispatcher.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-namespace-default.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-namespace-stack.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-atom10.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-rss20.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-rsscontent.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-dublincore.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-mediarss.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-yandex.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-rss11.c',
		'src/update_feed/parse_feed/parse_xml_feed/xml-handler-atom03.c',
		'src/update_feed/parse_feed/parse_json_feed/parse_json_feed.c',
		'src/update_feed/parse_feed/parse_json_feed/json-handler-jsonfeed.c',
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
	],
	dependencies: [
		dependency('ncursesw'),
		dependency('libcurl'),
		dependency('sqlite3'),
		dependency('tidy'),
		dependency('libcjson'),
	],
	#link_args: ['-static'],
	install: true,
)
