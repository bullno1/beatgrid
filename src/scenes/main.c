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
static float KEY_BUFFER = 0.05f;
static float CURSOR_MOVE_SPEED = 30.f;
static float MOUSE_THRESHOLD = 0.001f;

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main, TYPE, NAME)

SCENE_VAR(bg_pos_t, cursor_pos)
SCENE_VAR(CF_V2, cursor_smooth_pos)
SCENE_VAR(bg_grid_t, grid)

SCENE_VAR(CF_ButtonBinding, btn_cursor_up)
SCENE_VAR(CF_ButtonBinding, btn_cursor_down)
SCENE_VAR(CF_ButtonBinding, btn_cursor_left)
SCENE_VAR(CF_ButtonBinding, btn_cursor_right)
SCENE_VAR(CF_ButtonBinding, btn_del_sym)

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

	if (btn_cursor_up.id != 0) {
		cf_destroy_button_binding(btn_cursor_up);
		cf_destroy_button_binding(btn_cursor_down);
		cf_destroy_button_binding(btn_cursor_left);
		cf_destroy_button_binding(btn_cursor_right);
		cf_destroy_button_binding(btn_del_sym);
	}

	btn_cursor_up    = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_down  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_left  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_right = cf_make_button_binding(0, KEY_BUFFER);
	btn_del_sym      = cf_make_button_binding(0, KEY_BUFFER);

	cf_button_binding_set_repeat(btn_cursor_up   , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_down , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_left , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_right, KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_del_sym     , KEY_DELAY, KEY_REPEAT);

	cf_button_binding_add_key(btn_cursor_up   , CF_KEY_UP);
	cf_button_binding_add_key(btn_cursor_down , CF_KEY_DOWN);
	cf_button_binding_add_key(btn_cursor_left , CF_KEY_LEFT);
	cf_button_binding_add_key(btn_cursor_right, CF_KEY_RIGHT);
	cf_button_binding_add_key(btn_del_sym     , CF_KEY_BACKSPACE);
	cf_button_binding_add_key(btn_del_sym     , CF_KEY_DELETE);

	cf_input_enable_ime();
}

static void
cleanup(void) {
	bg_grid_cleanup(&grid);

	cf_destroy_button_binding(btn_cursor_up);
	cf_destroy_button_binding(btn_cursor_down);
	cf_destroy_button_binding(btn_cursor_left);
	cf_destroy_button_binding(btn_cursor_right);
	cf_destroy_button_binding(btn_del_sym);

	btn_cursor_up.id = 0;
	btn_cursor_down.id = 0;
	btn_cursor_left.id = 0;
	btn_cursor_right.id = 0;
	btn_del_sym.id = 0;

	cf_input_disable_ime();
}

static void
fixed_update(void* userdata) {
}

static CF_V2
grid_pos_to_world(bg_pos_t grid_pos) {
	return cf_mul(cf_v2(grid_pos.x, grid_pos.y), GRID_SIZE);
}

static void
update(void) {
	cf_app_update(fixed_update);

// Input {{{

	if (
		cf_button_binding_consume_press(btn_cursor_up)
		||
		cf_button_binding_repeated(btn_cursor_up)
	) {
		cursor_pos.y += 1;
	}

	if (
		cf_button_binding_consume_press(btn_cursor_down)
		||
		cf_button_binding_repeated(btn_cursor_down)
	) {
		cursor_pos.y -= 1;
	}

	if (
		cf_button_binding_consume_press(btn_cursor_left)
		||
		cf_button_binding_repeated(btn_cursor_left)
	) {
		cursor_pos.x -= 1;
	}

	if (
		cf_button_binding_consume_press(btn_cursor_right)
		||
		cf_button_binding_repeated(btn_cursor_right)
	) {
		cursor_pos.x += 1;
	}

	if (cf_button_binding_consume_press(btn_del_sym)) {
		bg_grid_del(&grid, cursor_pos);
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

	if (cf_input_text_has_data()) {
		int codepoint = cf_input_text_pop_utf32();
		cf_input_text_clear();

		if (codepoint >= 32 && codepoint <= 126 && codepoint != ' ') {
			bg_grid_put(&grid, cursor_pos, (bg_sym_t)codepoint);
		}
	}

// }}}

// Grid render {{{

	// Symbols
	cf_push_text_effect_active(false);
	cf_push_font_size(GRID_SIZE);
	for (bhash_index_t i = 0; i < bhash_len(&grid); ++i) {
		bg_pos_t pos = grid.keys[i];
		cf_push_text_id(bhash_hash(&pos, sizeof(pos)));

		char text_buf[2] = { grid.values[i] };
		CF_V2 text_size = cf_text_size(text_buf, 1);
		CF_V2 text_pos = grid_pos_to_world(pos);
		text_pos.x -= text_size.x * 0.5f;
		text_pos.y += text_size.y * 0.5f;

		cf_draw_text(text_buf, text_pos, 1);

		cf_pop_text_id();
	}
	cf_pop_font_size();
	cf_pop_text_effect_active();

	// Cursor
	CF_V2 cursor_target = grid_pos_to_world(cursor_pos);
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
	cf_draw_box_rounded(cursor, 0.5f, 1.2f);

// }}}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
};
