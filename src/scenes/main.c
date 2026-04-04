// vim: set foldmethod=marker foldlevel=0:
#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <cute.h>
#include <blog.h>
#include <SDL3/SDL_audio.h>
#include "../grid.h"
#include "../tribuf.h"

static float GRID_SIZE = 24.f;
static float KEY_DELAY = 0.2f;
static float KEY_REPEAT = 0.1f;
static float KEY_BUFFER = 0.00f;
static float CURSOR_MOVE_SPEED = 30.f;
static float MOUSE_THRESHOLD = 0.001f;

typedef struct {
	bg_grid_params_t grid_params;
	bg_grid_t grid;
} audio_cmd_t;

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main, TYPE, NAME)

SCENE_VAR(bg_pos_t, cursor_pos)
SCENE_VAR(CF_V2, cursor_smooth_pos)
SCENE_VAR(bg_grid_t, grid)
SCENE_VAR(bg_node_registry_t, node_registry)
SCENE_VAR(bg_pipeline_t*, editor_pipeline)
SCENE_VAR(bg_pipeline_params_t, pipeline_params)
SCENE_VAR(SDL_AudioStream*, audio_stream)
SCENE_VAR(bool, playing)

SCENE_VAR(audio_cmd_t*, audio_cmd_buf)
SCENE_VAR(tribuf_t, audio_cmd_queue)
SCENE_VAR(bg_pipeline_t*, audio_pipeline)
SCENE_VAR(float*, audio_buf)
SCENE_VAR(int, audio_buf_len)
static bg_output_t audio_outputs[1] = { 0 };

SCENE_VAR(CF_ButtonBinding, btn_cursor_up)
SCENE_VAR(CF_ButtonBinding, btn_cursor_down)
SCENE_VAR(CF_ButtonBinding, btn_cursor_left)
SCENE_VAR(CF_ButtonBinding, btn_cursor_right)
SCENE_VAR(CF_ButtonBinding, btn_del_sym)
SCENE_VAR(CF_ButtonBinding, btn_play_pause)

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
destroy_bindings(void) {
	cf_destroy_button_binding(btn_cursor_up);
	cf_destroy_button_binding(btn_cursor_down);
	cf_destroy_button_binding(btn_cursor_left);
	cf_destroy_button_binding(btn_cursor_right);
	cf_destroy_button_binding(btn_del_sym);
	cf_destroy_button_binding(btn_play_pause);
}

static void SDLCALL
audio_callback(
	void* userdata,
	SDL_AudioStream* stream,
    int additional_amount,
	int total_amount
) {
	static float last_sample = 0.f;

	audio_cmd_t* cmd = tribuf_begin_recv(&audio_cmd_queue);
	if (cmd != NULL) {
		bg_pipeline_set_params(audio_pipeline, (bg_pipeline_params_t){
			.dt = 1.f / 44100.f,
			.num_outputs = CF_ARRAY_SIZE(audio_outputs),
			.outputs = audio_outputs,
		});
		bg_pipeline_build(audio_pipeline, &node_registry, &cmd->grid);
		tribuf_end_recv(&audio_cmd_queue);
	}

	int num_frames_needed = additional_amount / sizeof(float);
	if (num_frames_needed > audio_buf_len) {
		audio_buf = bgame_realloc(audio_buf, sizeof(float) * num_frames_needed, scene_allocator);
		audio_buf_len = num_frames_needed;
	}

	for (int i = 0; i < num_frames_needed; ++i) {
		bg_pipeline_process(audio_pipeline);

		float sample = audio_outputs[0].value / (float)audio_outputs[0].count;
		if (!isnan(sample)) {  // NaN is emitted as reset signal
			audio_buf[i] = last_sample = sample;
		} else {
			audio_buf[i] = last_sample;
		}
	}

	SDL_PutAudioStreamData(stream, audio_buf, additional_amount);
}

static void
init(void) {
	cf_clear_color(0.0f, 0.0f, 0.0f, 0.0f);

	bg_grid_reinit(&grid, scene_allocator);

	bg_node_registry_reinit(&node_registry, scene_allocator);
	bg_pipeline_reinit(&editor_pipeline, scene_allocator);
	bg_pipeline_set_params(editor_pipeline, pipeline_params);
	bg_pipeline_build(editor_pipeline, &node_registry, &grid);

	if (bgame_current_scene_state() == BGAME_SCENE_INITIALIZING) {
		audio_cmd_buf = bgame_zalloc(sizeof(audio_cmd_t) * 3, scene_allocator);
		bg_grid_reinit(&audio_cmd_buf[0].grid, scene_allocator);
		bg_grid_reinit(&audio_cmd_buf[1].grid, scene_allocator);
		bg_grid_reinit(&audio_cmd_buf[2].grid, scene_allocator);
		tribuf_init(&audio_cmd_queue, audio_cmd_buf, sizeof(audio_cmd_t));

		bg_pipeline_reinit(&audio_pipeline, scene_allocator);
		bg_pipeline_set_params(audio_pipeline, pipeline_params);

		audio_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&(SDL_AudioSpec){
				.channels = 1,
				.freq = 44100,
				.format = SDL_AUDIO_F32,
			},
			audio_callback,
			NULL
		);
	}

	if (bgame_current_scene_state() == BGAME_SCENE_REINITIALIZING) {
		destroy_bindings();
	}

	btn_cursor_up    = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_down  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_left  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_right = cf_make_button_binding(0, KEY_BUFFER);
	btn_del_sym      = cf_make_button_binding(0, KEY_BUFFER);
	btn_play_pause   = cf_make_button_binding(0, KEY_BUFFER);

	cf_button_binding_set_repeat(btn_cursor_up   , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_down , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_left , KEY_DELAY, KEY_REPEAT);
	cf_button_binding_set_repeat(btn_cursor_right, KEY_DELAY, KEY_REPEAT);

	cf_button_binding_add_key(btn_cursor_up   , CF_KEY_UP);
	cf_button_binding_add_key(btn_cursor_down , CF_KEY_DOWN);
	cf_button_binding_add_key(btn_cursor_left , CF_KEY_LEFT);
	cf_button_binding_add_key(btn_cursor_right, CF_KEY_RIGHT);
	cf_button_binding_add_key(btn_del_sym     , CF_KEY_BACKSPACE);
	cf_button_binding_add_key(btn_del_sym     , CF_KEY_DELETE);
	cf_button_binding_add_key(btn_play_pause  , CF_KEY_SPACE);

	cf_input_enable_ime();
}

static void
cleanup(void) {
	SDL_DestroyAudioStream(audio_stream);
	bg_grid_cleanup(&audio_cmd_buf[0].grid);
	bg_grid_cleanup(&audio_cmd_buf[1].grid);
	bg_grid_cleanup(&audio_cmd_buf[2].grid);
	bgame_free(audio_cmd_buf, scene_allocator);
	bgame_free(audio_buf, scene_allocator);

	bg_pipeline_cleanup(&editor_pipeline);
	bg_pipeline_cleanup(&audio_pipeline);
	bg_node_registry_cleanup(&node_registry);
	bg_grid_cleanup(&grid);

	destroy_bindings();

	cf_input_disable_ime();
}

static void
before_reload(void) {
	SDL_SetAudioStreamGetCallback(audio_stream, NULL, NULL);
}

static void
after_reload(void) {
	bg_grid_reinit(&audio_cmd_buf[0].grid, scene_allocator);
	bg_grid_reinit(&audio_cmd_buf[1].grid, scene_allocator);
	bg_grid_reinit(&audio_cmd_buf[2].grid, scene_allocator);

	bg_node_registry_reinit(&node_registry, scene_allocator);
	bg_pipeline_reinit(&audio_pipeline, scene_allocator);
	bg_pipeline_build(audio_pipeline, &node_registry, &grid);
	bg_pipeline_set_params(audio_pipeline, (bg_pipeline_params_t){
		.dt = 1.f / 48000.f,
		.num_outputs = CF_ARRAY_SIZE(audio_outputs),
		.outputs = audio_outputs,
	});

	SDL_SetAudioStreamGetCallback(audio_stream, audio_callback, NULL);
}

static void
fixed_update(void* userdata) {
}

static CF_V2
grid_pos_to_world(bg_pos_t grid_pos) {
	return cf_mul(cf_v2(grid_pos.x, grid_pos.y), GRID_SIZE);
}

static void
send_audio_state(void) {
	audio_cmd_t* cmd = tribuf_begin_send(&audio_cmd_queue);

	bhash_clear(&cmd->grid);
	for (bhash_index_t i = 0; i < bhash_len(&grid); ++i) {
		bhash_put(&cmd->grid, grid.keys[i], grid.values[i]);
	}

	tribuf_end_send(&audio_cmd_queue);
}

static void
update(void) {
	tribuf_try_swap(&audio_cmd_queue);

	cf_app_update(fixed_update);

// Input {{{

// Cursor {{{
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

// Edit {{{
	bool grid_modified = false;

	if (cf_button_binding_consume_press(btn_del_sym)) {
		bg_grid_del(&grid, cursor_pos);
		grid_modified = true;
	}

	if (cf_input_text_has_data()) {
		int codepoint = cf_input_text_pop_utf32();
		cf_input_text_clear();

		if (codepoint >= 32 && codepoint <= 126 && codepoint != ' ') {
			bg_grid_put(&grid, cursor_pos, (bg_sym_t)codepoint);
			grid_modified = true;
		}
	}

	if (grid_modified) {
		bg_pipeline_build(editor_pipeline, &node_registry, &grid);
		send_audio_state();
	}
// }}}

// Control {{{
	if (cf_button_binding_consume_press(btn_play_pause)) {
		playing = !playing;

		if (playing) {
			SDL_ResumeAudioStreamDevice(audio_stream);
		} else {
			SDL_PauseAudioStreamDevice(audio_stream);
		}
	}
// }}}

// }}}

// Grid render {{{

	// Symbols
	for (bhash_index_t i = 0; i < bhash_len(&grid); ++i) {
		bg_pos_t pos = grid.keys[i];

		const bg_node_t* node = bg_node_registry_lookup(&node_registry, grid.values[i]);
		if (node == NULL) { continue; }
	}

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
	CF_Aabb cursor_box = cf_make_aabb_pos_w_h(
		cursor_smooth_pos,
		GRID_SIZE, GRID_SIZE
	);
	cf_draw_box_rounded(cursor_box, 0.5f, 1.2f);

	// I/O lines
	int num_edges = bg_pipeline_count_edges(editor_pipeline);
	const bg_edge_t* edges = bg_pipeline_get_edges(editor_pipeline);
	for (int i = 0; i < num_edges; ++i) {
		CF_V2 from_pos = grid_pos_to_world(edges[i].from);
		CF_V2 to_pos   = grid_pos_to_world(edges[i].to);

		if (
			cf_contains_point(cursor_box, from_pos)
			||
			cf_contains_point(cursor_box, to_pos)
		) {
			cf_draw_line(from_pos, to_pos, 0.5f);
		}
	}

// }}}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
	.before_reload = before_reload,
	.after_reload = after_reload,
};
