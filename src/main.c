#include <bgame/entrypoint.h>
#include <bgame/allocator.h>
#include <bgame/scene.h>
#include <blog.h>
#include <cute.h>

#if BGAME_RELOADABLE
#include <bent.h>
#endif

static const char* WINDOW_TITLE = "gridbeat";
BGAME_VAR(bool, app_created) = false;

static void
init(int argc, const char** argv) {
	// Cute Framework
	if (!app_created) {
		BLOG_INFO("Creating app");
		int options =
			  CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT
			| CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT_BIT;
		CF_Result result = cf_make_app(WINDOW_TITLE, 0, 0, 0, 1280, 720, options, argv[0]);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_FATAL("Could not create app: %s", result.details);
			abort();
		}

		cf_app_init_imgui();
		app_created = true;
	}

	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);
	cf_app_set_title(WINDOW_TITLE);

	if (bgame_current_scene() == NULL) {
		bgame_push_scene("main");
		bgame_scene_update();
	}
}

static void
cleanup(void) {
	bgame_clear_scene_stack();
	cf_destroy_app();
}

static bgame_app_t app = {
	.init = init,
	.cleanup = cleanup,
	.update = bgame_scene_update,
	.before_reload = bgame_scene_before_reload,
	.after_reload = bgame_scene_after_reload,
};

BGAME_ENTRYPOINT(app)
