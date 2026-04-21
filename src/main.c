#include <bgame/entrypoint.h>
#include <bgame/allocator.h>
#include <bgame/allocator/tracked.h>
#include <bgame/scene.h>
#include <bgame/asset.h>
#include <blog.h>
#include <cute.h>

#ifdef __EMSCRIPTEN__

#include <emscripten/html5.h>

EM_JS(void, get_html_window_size, (int* width, int* height), {
	HEAP32[width  >> 2] = window.innerWidth;
	HEAP32[height >> 2] = window.innerHeight;
});

#endif

static const char* WINDOW_TITLE = "beatgrid";
BGAME_VAR(bool, app_created) = false;
BGAME_VAR(bgame_asset_bundle_t*, predefined_assets) = { 0 };

static void
load_assets(void) {
	bgame_asset_init(&predefined_assets, bgame_default_allocator);
	BGAME_FOREACH_DEFINED_ASSET(asset) {
		// Optional tag filtering
		bgame_asset_load_def(predefined_assets, asset);
	}
}

static void
report_allocator_stats(
	const char* name,
	bgame_allocator_stats_t stats,
	void* userdata
) {
	BLOG_DEBUG("%s: Total %zu, Peak %zu", name, stats.total, stats.peak);
}

static void
init(int argc, const char** argv) {
	// Cute Framework
	if (!app_created) {
		int width  = 1280;
		int height = 768;

#ifdef __EMSCRIPTEN__
		get_html_window_size(&width, &height);
		BLOG_INFO("Canvas size: %d x %d", width, height);
#endif

		BLOG_INFO("Creating app");
		int options =
			  CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT
			| CF_APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT_BIT
			| CF_APP_OPTIONS_RESIZABLE_BIT
			;
		CF_Result result = cf_make_app(WINDOW_TITLE, 0, 0, 0, width, height, options, argv[0]);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_FATAL("Could not create app: %s", result.details);
			abort();
		}

		result = cf_fs_mount("./assets", "/assets", true);
		if (result.code != CF_RESULT_SUCCESS) {
			BLOG_WARN("Could not mount %s: %s", ".", result.details);
		}

		cf_app_init_imgui();

		app_created = true;
	}

	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);

	load_assets();

	if (bgame_current_scene() == NULL) {
		bgame_push_scene("main");
		bgame_scene_update();
	}
}

static void
update(void) {
	if (cf_app_was_resized()) {
		int width = cf_app_get_width();
		int height = cf_app_get_height();
		cf_app_set_canvas_size(width, height);
		cf_draw_projection(cf_ortho_2d(0, 0, (float)width, (float)height));
	}

	bgame_scene_update();
}

static void
cleanup(void) {
	bgame_clear_scene_stack();
	cf_destroy_app();

	BLOG_DEBUG("--- Allocator stats ---");
	bgame_enumerate_tracked_allocators(report_allocator_stats, NULL);
}

static void
after_reload(void) {
	load_assets();
	bgame_scene_after_reload();
}

static bgame_app_t app = {
	.init = init,
	.cleanup = cleanup,
	.update = update,
	.before_reload = bgame_scene_before_reload,
	.after_reload = after_reload,
};

BGAME_ENTRYPOINT(app)
