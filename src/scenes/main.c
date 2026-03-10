#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <cute.h>
#include <blog.h>

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main, TYPE, NAME)

BGAME_DECLARE_SCENE_ALLOCATOR(main)

static void
init(void) {
	cf_clear_color(0.5f, 0.5f, 0.5f, 0.0f);
}

static void
cleanup(void) {
}

static void
fixed_update(void* userdata) {
}

static void
update(void) {
	cf_app_update(fixed_update);

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
