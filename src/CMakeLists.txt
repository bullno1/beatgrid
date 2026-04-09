set(SOURCES
	"main.c"
	"grid.c"
	"assets.c"

	"scenes/main.c"

	"nodes/oscillators.c"
	"nodes/io.c"
	"nodes/arithmetic.c"
	"nodes/wiring.c"
)
add_bgame_app(beatgrid "${SOURCES}")

if (EMSCRIPTEN)
	target_link_libraries(beatgrid PRIVATE bgame-emscripten-shell)
endif ()
