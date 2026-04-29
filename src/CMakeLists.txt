set(SOURCES
	"main.c"
	"grid.c"
	"assets.c"
	"audio_callback.c"
	"ufa.c"
	"history.c"

	"scenes/main.c"

	"nodes/oscillators.c"
	"nodes/io.c"
	"nodes/arithmetic.c"
	"nodes/wiring.c"
)
add_bgame_app(beatgrid "${SOURCES}")

if (EMSCRIPTEN)
	add_custom_target(copy-beatgrid-emscripten-shell
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/emscripten ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/emscripten
	)
	add_library(beatgrid-emscripten-shell INTERFACE)
	add_dependencies(beatgrid-emscripten-shell copy-beatgrid-emscripten-shell)
	target_link_options(beatgrid-emscripten-shell INTERFACE --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell.html")

	target_link_libraries(beatgrid PRIVATE beatgrid-emscripten-shell)
	target_link_options(beatgrid PRIVATE
		"--js-library=${CMAKE_CURRENT_LIST_DIR}/ufa.js"
		-sMALLOC=emmalloc
		-gseparate-dwarf
		--preload-file "${CMAKE_CURRENT_LIST_DIR}/../assets@/assets"
	)
endif ()
