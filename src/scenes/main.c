// vim: set foldmethod=marker foldlevel=0:
#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <cute.h>
#include <blog.h>
#include "../grid.h"

static float GRID_SIZE = 24.f;
static float KEY_DELAY = 0.1f;
static float KEY_REPEAT = 0.1f;
static float CURSOR_MOVE_SPEED = 30.f;
static float MOUSE_THRESHOLD = 0.001f;

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main, TYPE, NAME)

SCENE_VAR(bg_pos_t, cursor_pos)
SCENE_VAR(CF_V2, cursor_smooth_pos)
SCENE_VAR(bg_grid_t, grid)

SCENE_VAR(CF_ButtonBinding, cursor_up)
SCENE_VAR(CF_ButtonBinding, cursor_down)
SCENE_VAR(CF_ButtonBinding, cursor_left)
SCENE_VAR(CF_ButtonBinding, cursor_right)

BGAME_DECLARE_SCENE_ALLOCATOR(main)

static bool
on_repeated_key(CF_KeyButton key, double* last_down_seconds) {
	if (cf_key_just_pressed(key)) {
		*last_down_seconds = CF_SECONDS;
		return true;
	} else if (
		cf_on_interval(0.5f, CF_SECONDS - *last_down_seconds)
		&&
		cf_key_down(key)
	) {
		return true;
	} else {
		return false;
	}
}

static void
init(void) {
	cf_clear_color(0.0f, 0.0f, 0.0f, 0.0f);

	bg_grid_reinit(&grid, scene_allocator);

	if (cursor_up.id != 0) {
		cf_destroy_button_binding(cursor_up);
		cf_destroy_button_binding(cursor_down);
		cf_destroy_button_binding(cursor_left);
		cf_destroy_button_binding(cursor_right);
	}

	cursor_up    = cf_make_button_binding(0, 0.05);
	cursor_down  = cf_make_button_binding(0, 0.05);
	cursor_left  = cf_make_button_binding(0, 0.05);
	cursor_right = cf_make_button_binding(0, 0.05);

	cf_button_binding_set_repeat(cursor_up   , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(cursor_down , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(cursor_left , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(cursor_right, KEY_DELAY, KEY_REPEAT);

	cf_button_binding_add_key(cursor_up   , CF_KEY_UP);
	cf_button_binding_add_key(cursor_down , CF_KEY_DOWN);
	cf_button_binding_add_key(cursor_left , CF_KEY_LEFT);
	cf_button_binding_add_key(cursor_right, CF_KEY_RIGHT);
}

static void
cleanup(void) {
	bg_grid_cleanup(&grid);

	cf_destroy_button_binding(cursor_up);
	cf_destroy_button_binding(cursor_down);
	cf_destroy_button_binding(cursor_left);
	cf_destroy_button_binding(cursor_right);

	cursor_up.id = 0;
	cursor_down.id = 0;
	cursor_left.id = 0;
	cursor_right.id = 0;
}

static void
fixed_update(void* userdata) {
}

static void
update(void) {
	cf_app_update(fixed_update);

// Input {{{
	if (
		cf_button_binding_consume_press(cursor_up)
		||
		cf_button_binding_repeated(cursor_up)
	) {
		cursor_pos.y += 1;
	}

	if (
		cf_button_binding_consume_press(cursor_down)
		||
		cf_button_binding_repeated(cursor_down)
	) {
		cursor_pos.y -= 1;
	}

	if (
		cf_button_binding_consume_press(cursor_left)
		||
		cf_button_binding_repeated(cursor_left)
	) {
		cursor_pos.x -= 1;
	}

	if (
		cf_button_binding_consume_press(cursor_right)
		||
		cf_button_binding_repeated(cursor_right)
	) {
		cursor_pos.x += 1;
	}

	if (
		cf_abs(cf_mouse_motion_x()) > MOUSE_THRESHOLD
		||
		cf_abs(cf_mouse_motion_y()) > MOUSE_THRESHOLD
	) {
		CF_V2 p = cf_screen_to_world(cf_v2((float)cf_mouse_x(), (float)cf_mouse_y()));
		p = cf_div(p, cf_v2(GRID_SIZE, GRID_SIZE));
		cursor_pos.x = cf_round(p.x);
		cursor_pos.y = cf_round(p.y);
	}

// }}}

// Grid render {{{

	CF_V2 cursor_target = cf_v2(GRID_SIZE * (float)cursor_pos.x, GRID_SIZE * (float)cursor_pos.y);
	cursor_smooth_pos = cf_add(
		cursor_smooth_pos,
		cf_mul(
			cf_sub(cursor_target, cursor_smooth_pos),
			(1 - cf_exp(- CURSOR_MOVE_SPEED * CF_DELTA_TIME))
		)
	);
	CF_Aabb cursor = cf_make_aabb_pos_w_h(
		cursor_smooth_pos,
		GRID_SIZE, GRID_SIZE
	);
	cf_draw_box_rounded(cursor, 0.5f, 0.5f);

// }}}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
