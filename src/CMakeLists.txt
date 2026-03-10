set(SOURCES
	"main.c"
	"scenes/main.c"
)
add_bgame_app(beatgrid "${SOURCES}")

if (EMSCRIPTEN)
	target_link_libraries(beatgrid PRIVATE bgame-emscripten-shell)
endif ()
