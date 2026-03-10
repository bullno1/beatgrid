set(SOURCES
	"main.c"
	"scenes/main.c"
)
add_bgame_app(gridbeat "${SOURCES}")

if (EMSCRIPTEN)
	target_link_libraries(gridbeat PRIVATE bgame-emscripten-shell)
endif ()
