set(SOURCES
	"main.c"
	"grid.c"

	"scenes/main.c"

	"nodes/oscillators.c"
)
add_bgame_app(beatgrid "${SOURCES}")

if (EMSCRIPTEN)
	target_link_libraries(beatgrid PRIVATE bgame-emscripten-shell)
endif ()
