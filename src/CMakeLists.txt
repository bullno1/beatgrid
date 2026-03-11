set(SOURCES
	"main.c"
	"grid.c"

	"scenes/main.c"

	"operators/generators.c"
)
add_bgame_app(beatgrid "${SOURCES}")
target_link_libraries(beatgrid PRIVATE nanbox)

if (EMSCRIPTEN)
	target_link_libraries(beatgrid PRIVATE bgame-emscripten-shell)
endif ()
