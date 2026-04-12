// vim: set foldmethod=marker foldlevel=0:
#include <bgame/scene.h>
#include <bgame/allocator/tracked.h>
#include <bgame/reloadable.h>
#include <bgame/ui.h>
#include <bgame/ui/spacer.h>
#include <bgame/ui/string.h>
#include <bgame/ui/rich_text.h>
#include <bgame/utils.h>
#include <cute.h>
#include <blog.h>
#include "../assets.h"
#include <SDL3/SDL_audio.h>
#include "../grid.h"
#include "../tribuf.h"
#include "../audio_callback.h"

static float GRID_SIZE = 24.f;
static float KEY_DELAY = 0.2f;
static float KEY_REPEAT = 0.1f;
static float KEY_BUFFER = 0.00f;
static float CURSOR_MOVE_SPEED = 30.f;
static int SAMPLE_RATE = 44100;

enum {
	FONT_DEFAULT,
	FONT_GRID,
	FONT_CHROME,
};

#define SCENE_VAR(TYPE, NAME) BGAME_PRIVATE_VAR(main, TYPE, NAME)

SCENE_VAR(bg_pos_t, cursor_pos)
SCENE_VAR(CF_V2, cursor_smooth_pos)
SCENE_VAR(bg_grid_t, grid)
SCENE_VAR(bg_node_registry_t, node_registry)
SCENE_VAR(bg_pipeline_t*, editor_pipeline)
SCENE_VAR(bg_pipeline_params_t, pipeline_params)
SCENE_VAR(SDL_AudioStream*, audio_stream)
SCENE_VAR(bool, playing)

SCENE_VAR(audio_callback_state_t*, audio_callback_state);

SCENE_VAR(CF_ButtonBinding, btn_cursor_up)
SCENE_VAR(CF_ButtonBinding, btn_cursor_down)
SCENE_VAR(CF_ButtonBinding, btn_cursor_left)
SCENE_VAR(CF_ButtonBinding, btn_cursor_right)
SCENE_VAR(CF_ButtonBinding, btn_del_sym)
SCENE_VAR(CF_ButtonBinding, btn_play_pause)
SCENE_VAR(CF_ButtonBinding, btn_scroll_multiply)

SCENE_VAR(bool, right_sidebar_enabled)

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
	cf_destroy_button_binding(btn_scroll_multiply);
}

static void
send_audio_state(void) {
	audio_cmd_t* cmd = audio_callback_control_begin(audio_callback_state);

	bhash_clear(&cmd->grid);
	for (bhash_index_t i = 0; i < bhash_len(&grid); ++i) {
		bhash_put(&cmd->grid, grid.keys[i], grid.values[i]);
	}
	cmd->grid_params = pipeline_params.grid_params;
	cmd->playing = playing;

	audio_callback_control_end(audio_callback_state);
}

static void
init(void) {
	cf_clear_color(0.0f, 0.0f, 0.0f, 0.0f);

	if (bgame_current_scene_state() == BGAME_SCENE_INITIALIZING) {
		pipeline_params = (bg_pipeline_params_t){
			.dt = 1.f / (float)SAMPLE_RATE,
			.grid_params = {
				.bpm = 120,
			},
		};
		audio_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
			&(SDL_AudioSpec){
				.channels = 1,
				.freq = SAMPLE_RATE,
				.format = SDL_AUDIO_F32,
			},
			NULL,
			NULL
		);
	}

	bg_grid_reinit(&grid, scene_allocator);
	bg_node_registry_reinit(&node_registry, scene_allocator);
	bg_pipeline_reinit(&editor_pipeline, scene_allocator);
	bg_pipeline_set_params(editor_pipeline, pipeline_params);
	bg_pipeline_build(editor_pipeline, &node_registry, &grid);

	SDL_AudioSpec spec;
	int frames_per_call;
	SDL_GetAudioDeviceFormat(
		SDL_GetAudioStreamDevice(audio_stream),
		&spec,
		&frames_per_call
	);
	audio_callback_reinit(&audio_callback_state, (audio_callback_config_t){
		.allocator = scene_allocator,
		.grid_param = pipeline_params.grid_params,
		.node_registry = &node_registry,
		.sample_rate = spec.freq,
		.frames_per_call = frames_per_call,
	});
	SDL_SetAudioStreamGetCallback(audio_stream, audio_callback, audio_callback_state);
	SDL_ResumeAudioStreamDevice(audio_stream);
	send_audio_state();

	if (bgame_current_scene_state() == BGAME_SCENE_REINITIALIZING) {
		destroy_bindings();
	}

	btn_cursor_up    = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_down  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_left  = cf_make_button_binding(0, KEY_BUFFER);
	btn_cursor_right = cf_make_button_binding(0, KEY_BUFFER);
	btn_del_sym      = cf_make_button_binding(0, KEY_BUFFER);
	btn_play_pause   = cf_make_button_binding(0, KEY_BUFFER);
	btn_scroll_multiply = cf_make_button_binding(0, KEY_BUFFER);

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
	cf_button_binding_add_key(btn_scroll_multiply, CF_KEY_LSHIFT);
	cf_button_binding_add_key(btn_scroll_multiply, CF_KEY_RSHIFT);

	cf_input_enable_ime();

	bgame_register_ui_font(FONT_GRID, font_grid->name);
	bgame_register_ui_font(FONT_CHROME, font_chrome->name);
}

static void
cleanup(void) {
	SDL_DestroyAudioStream(audio_stream);
	audio_callback_cleanup(&audio_callback_state);

	bg_pipeline_cleanup(&editor_pipeline);
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
fixed_update(void* userdata) {
}

static CF_V2
grid_pos_to_world(bg_pos_t grid_pos) {
	return cf_mul(cf_v2(grid_pos.x, grid_pos.y), GRID_SIZE);
}

static bg_pos_t
world_pos_to_grid_pos(CF_V2 world_pos) {
	return (bg_pos_t){
		.x = cf_round(world_pos.x / GRID_SIZE),
		.y = cf_round(world_pos.y / GRID_SIZE),
	};
}

// Grid {{{

static const int LIGHT_RADIUS = 10;

static float
grid_element_alpha(bg_pos_t pos, bg_pos_t cursor) {
	float distance = cf_distance(cf_v2(cursor_pos.x, cursor_pos.y), cf_v2(pos.x, pos.y));
	if (distance > LIGHT_RADIUS) {
		return 0.f;
	} else {
		return 1.f - distance / (float)LIGHT_RADIUS;
	}
}

static void
grid_element(const Clay_RenderCommand* command, void* userdata) {
	const int LIGHT_RADIUS = 10;

	BGAME_SCOPE(cf_draw_push(), cf_draw_pop())
	BGAME_SCOPE(cf_push_text_effect_active(false), cf_pop_text_effect_active())
	BGAME_SCOPE(cf_push_font_size(GRID_SIZE), cf_pop_font_size())
	BGAME_SCOPE(cf_push_font(font_grid->name), cf_pop_font())
	{
		const CF_V2 text_size = cf_text_size("X", 1);

		cf_draw_translate(command->boundingBox.x, -command->boundingBox.y - command->boundingBox.height);

		// Grid
		for (int x = cursor_pos.x - LIGHT_RADIUS; x <= cursor_pos.x + LIGHT_RADIUS; ++x) {
			for (int y = cursor_pos.y - LIGHT_RADIUS; y <= cursor_pos.y + LIGHT_RADIUS; ++y) {
				bg_pos_t pos = { x, y };
				if (bhash_has(&grid, pos)) { continue; }

				float distance = cf_distance(cf_v2(cursor_pos.x, cursor_pos.y), cf_v2(x, y));
				if (distance > LIGHT_RADIUS) { continue; }

				float alpha = grid_element_alpha((bg_pos_t){ x, y }, cursor_pos);
				if (alpha == 0.f) { continue; }

				CF_V2 grid_center = grid_pos_to_world(pos);
				cf_draw_push_color(cf_make_color_rgba_f(1.f, 1.f, 1.f, alpha));
				cf_draw_circle_fill2(grid_center, 0.2f);
				cf_draw_pop_color();
			}
		}

		// I/O lines
		int num_edges = bg_pipeline_count_edges(editor_pipeline);
		const bg_edge_t* edges = bg_pipeline_get_edges(editor_pipeline);
		for (int i = 0; i < num_edges; ++i) {
			float alpha = cf_max(
				grid_element_alpha(edges[i].from, cursor_pos),
				grid_element_alpha(edges[i].to  , cursor_pos)
			);
			CF_V2 from_pos = grid_pos_to_world(edges[i].from);
			CF_V2 to_pos   = grid_pos_to_world(edges[i].to);

			int bpm = playing ? pipeline_params.grid_params.bpm : 20;
			float beat_duration = 60.0f / (float)bpm;
			float lerp_factor = fmodf(CF_SECONDS, beat_duration) / beat_duration;
			CF_V2 particle_pos = cf_lerp(from_pos, to_pos, lerp_factor);

			if (playing) {
				cf_draw_circle_fill2(particle_pos, 1.2f);
			} else if (alpha > 0.f) {
				cf_draw_push_color(cf_make_color_rgba_f(1.f, 1.f, 1.f, alpha));
				cf_draw_circle_fill2(particle_pos, 1.f);
				cf_draw_pop_color();
			}

			if (alpha == 0.f) { continue; }

			cf_draw_push_color(cf_make_color_rgba_f(1.f, 1.f, 1.f, alpha * 0.8f));
			cf_draw_line(from_pos, to_pos, 0.5f);
			cf_draw_pop_color();
		}

		// Symbols
		for (bhash_index_t i = 0; i < bhash_len(&grid); ++i) {
			bg_pos_t pos = grid.keys[i];
			cf_push_text_id(bhash_hash(&pos, sizeof(pos)));

			bg_sym_t symbol = grid.values[i];

			char text_buf[2] = { symbol, 0 };
			CF_V2 text_pos = grid_pos_to_world(pos);
			CF_V2 glow_pos = text_pos;
			text_pos.x -= text_size.x * 0.5f;
			text_pos.y += text_size.y * 0.5f;

			CF_Color color = cf_make_color_rgb(0, 102, 34);
			bool glow = false;
			if (('0' <= symbol && symbol <= '9') || ('a' <= symbol && symbol <= 'z')) {
				color = cf_make_color_rgb(0, 255, 65);
				glow = true;
			} else if (bhash_has(&node_registry, symbol)) {
				color = cf_make_color_rgb(0, 245, 255);
				glow = true;
			}

			float alpha = grid_element_alpha(pos, cursor_pos);
			if (glow && alpha > 0.f) {
				BGAME_SCOPE(
					cf_draw_push_color(cf_make_color_rgba_f(0.f, 0.f, 0.f, alpha)),
					cf_draw_pop_color()
				)
				{
					cf_draw_circle_fill2(glow_pos, GRID_SIZE * 0.25f);
				}
			}

			BGAME_SCOPE(cf_draw_push_color(color), cf_draw_pop_color())
			{
				cf_draw_text(text_buf, text_pos, 1);
			}

			cf_pop_text_id();
		}

		// Cursor
		CF_V2 cursor_target = grid_pos_to_world(cursor_pos);
		CF_Aabb cursor_box = cf_make_aabb_pos_w_h(
			cursor_target,
			GRID_SIZE, GRID_SIZE
		);
		cf_draw_box_rounded(cursor_box, 0.5f, 1.2f);

		cursor_smooth_pos = cf_add(
			cursor_smooth_pos,
			cf_mul(
				cf_sub(cursor_target, cursor_smooth_pos),
				(1 - cf_exp(- CURSOR_MOVE_SPEED * CF_DELTA_TIME))
			)
		);
		CF_Aabb cursor_trail = cf_make_aabb_pos_w_h(
			cursor_smooth_pos,
			GRID_SIZE, GRID_SIZE
		);
		cf_draw_push_color(cf_make_color_rgba(255, 255, 255, 200));
		cf_draw_box_rounded(cursor_trail, 0.5f, 1.2f);
		cf_draw_pop_color();

		if (
			Clay_PointerOver((Clay_ElementId){ .id = command->id })
			&&
			cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)
		) {
			CF_V2 p = cf_screen_to_world(cf_v2((float)cf_mouse_x(), (float)cf_mouse_y()));
			p = cf_div(p, cf_v2(GRID_SIZE, GRID_SIZE));
			cursor_pos.x = cf_round(p.x);
			cursor_pos.y = cf_round(p.y);
		}
	}
}

// }}}

static int
cmp_by_category(const void* lhs_v, const void* rhs_v) {
	const bg_node_t* lhs = *(const bg_node_t**)lhs_v;
	const bg_node_t* rhs = *(const bg_node_t**)rhs_v;

	return strcmp(lhs->category, rhs->category);
}


static Clay_TransitionData
slide_right(Clay_TransitionData targetState, Clay_TransitionProperty properties) {
	Clay_TransitionData target = targetState;
	target.boundingBox.x += target.boundingBox.width;
	target.overlayColor.r = 0.01f;
	target.overlayColor.g = 0.01f;
	target.overlayColor.b = 0.01f;
	target.overlayColor.a = 0.01f;
	return target;
}

static void
update(void) {
	audio_callback_sync(audio_callback_state);

	cf_app_update(fixed_update);

	bool grid_modified = false;
	bool should_send_audio_state = false;

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
	// }}}

	// Edit {{{
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
	// }}}

	// Control {{{
	if (cf_button_binding_consume_press(btn_play_pause)) {
		playing = !playing;
		should_send_audio_state = true;
	}

	if (cf_key_just_pressed(CF_KEY_F12)) {
		Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
	}
	// }}}

	// }}}

	// UI {{{

	const Clay_Color UI_BORDER_COLOR = bgame_ui_color_rgb(0, 64, 26);
	const Clay_Color UI_TEXT_COLOR = bgame_ui_color_rgb(0, 255, 65);
	const Clay_Color UI_BACKGROUND_COLOR = bgame_ui_color_rgb(4, 12, 5);
	const Clay_Color UI_HOVER_COLOR = bgame_ui_color_rgb(0, 61, 15);
	const float UI_TRANSITION_DURATION = 0.25f;

	Clay_TransitionElementConfig transition_common = {
		.duration = UI_TRANSITION_DURATION,
		.handler = bgame_ui_transition,
		.userData = bgame_make_frame_copy(((bgame_ui_transition_config_t){
			.curve = cf_sin_in_out,
		})),
		.properties =
			  CLAY_TRANSITION_PROPERTY_POSITION
			| CLAY_TRANSITION_PROPERTY_BACKGROUND_COLOR
			| CLAY_TRANSITION_PROPERTY_OVERLAY_COLOR
			,
	};

	bgame_update_ui();
	Clay_BeginLayout();
	CLAY(CLAY_ID("Root"), {
		.layout = {
			.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
			.layoutDirection = CLAY_TOP_TO_BOTTOM,
		},
	}) {
		// Menu bar {{{
		const int MENU_BAR_FONT_SIZE = 16;
		CLAY(CLAY_ID("Top"), {
			.layout = {
				.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
			},
			.border = {
				.color = UI_BORDER_COLOR,
				.width = { .bottom = 1 },
			},
			.backgroundColor = UI_BACKGROUND_COLOR,
		}) {
#define MENU_ENTRY(NAME) \
			CLAY(CLAY_ID(#NAME), { \
				.layout.padding = { .left = 5, .right = 5, .top = 5, .bottom = 5 }, \
				.border = { \
					.color = UI_BORDER_COLOR, \
					.width = { .right = 1 }, \
				}, \
				.backgroundColor = Clay_Hovered() ? UI_HOVER_COLOR : UI_BACKGROUND_COLOR, \
				.transition = transition_common, \
			})

			MENU_ENTRY(File) {
				CLAY_TEXT(CLAY_STRING("FILE"), {
					.fontId = FONT_CHROME,
					.fontSize = MENU_BAR_FONT_SIZE,
					.textColor = UI_TEXT_COLOR,
					.textAlignment = CLAY_TEXT_ALIGN_CENTER,
				});
			}

			MENU_ENTRY(Options) {
				CLAY_TEXT(CLAY_STRING("OPTIONS"), {
					.fontId = FONT_CHROME,
					.fontSize = MENU_BAR_FONT_SIZE,
					.textColor = UI_TEXT_COLOR,
					.textAlignment = CLAY_TEXT_ALIGN_CENTER,
				});
			}

			MENU_ENTRY(Help) {
				CLAY_TEXT(CLAY_STRING("HELP"), {
					.fontId = FONT_CHROME,
					.fontSize = MENU_BAR_FONT_SIZE,
					.textColor = UI_TEXT_COLOR,
					.textAlignment = CLAY_TEXT_ALIGN_CENTER,
				});
			}
		}
		// }}}

		// Main {{{
		CLAY(CLAY_ID("Mid"), {
			.layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
		}) {
			CLAY(CLAY_ID("GridContainer"), {
				.layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
				.clip = { .vertical = true, .horizontal = true }
			}) {
				CLAY(CLAY_ID("Grid"), {
					.layout.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
					.custom = bgame_custom_ui_element(grid_element, NULL),
				}) {
				}
			}

			CLAY(CLAY_ID("RightSidebar"), {
				.layout.sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0) },
				.border = {
					.color = UI_BORDER_COLOR,
					.width = CLAY_BORDER_ALL(1)
				},
				.backgroundColor = UI_BACKGROUND_COLOR,
				.transition = transition_common,
			}) {
				CLAY(CLAY_ID_LOCAL("PullTab"), {
					.layout = {
						.sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0) },
						.layoutDirection = CLAY_TOP_TO_BOTTOM,
						.childAlignment.y = CLAY_ALIGN_Y_CENTER,
						.padding = { .left = 1, .right = 1 },
					},
					.transition = transition_common,
					.backgroundColor = Clay_Hovered() ? UI_HOVER_COLOR : UI_BACKGROUND_COLOR, \
				}) {
					CLAY_TEXT(right_sidebar_enabled ? CLAY_STRING(">") : CLAY_STRING("<"), {
						.fontId = FONT_CHROME,
						.fontSize = MENU_BAR_FONT_SIZE,
						.textColor = UI_TEXT_COLOR,
						.userData = bgame_ui_text_config((bgame_ui_text_config_t){
							.effect = BGAME_UI_DISABLE_TEXT_EFFECT,
						})
					});

					if (Clay_Hovered() && cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
						right_sidebar_enabled = !right_sidebar_enabled;
					}
				}

				if (right_sidebar_enabled) {
					bhash_index_t num_nodes = bhash_len(&node_registry);
					const bg_node_t** node_types = bgame_alloc_for_frame(
						sizeof(bg_node_t*) * num_nodes, _Alignof(bg_node_t)
					);
					for (bhash_index_t i = 0; i < bhash_len(&node_registry); ++i) {
						node_types[i] = node_registry.values[i];
					}
					qsort(node_types, num_nodes, sizeof(node_types[0]), cmp_by_category);

					CLAY(CLAY_ID_LOCAL("Content"), {
						.layout = {
							.sizing = { CLAY_SIZING_FIXED(300), CLAY_SIZING_GROW(0) },
							.layoutDirection = CLAY_TOP_TO_BOTTOM,
						},
						.clip = {
							.vertical = true,
							.childOffset = Clay_GetScrollOffset(),
						},
						.overlayColor = bgame_ui_color_from_cf(cf_color_white()),
						.transition = {
							.duration = UI_TRANSITION_DURATION,
							.handler = bgame_ui_transition,
							.userData = bgame_make_frame_copy(((bgame_ui_transition_config_t){
								.curve = cf_sin_in_out,
							})),
							.enter.setInitialState = slide_right,
							.exit.setFinalState = slide_right,
							.properties = CLAY_TRANSITION_PROPERTY_POSITION | CLAY_TRANSITION_PROPERTY_OVERLAY_COLOR,
						},
					}) {
						const char* last_category = "";
						bool first_category = true;

						for (bhash_index_t i = 0; i < num_nodes; ++i) {
							const bg_node_t* node = node_types[i];

							if (strcmp(node->category, last_category) != 0) {  // New category
								last_category = node->category;

								if (first_category) {
									first_category = false;
								} else {
									Clay__CloseElement();
								}

								Clay_String clay_category = {
									.isStaticallyAllocated = true,
									.chars =  last_category,
									.length = strlen(last_category)
								};
								Clay__OpenElementWithId(CLAY_SID_LOCAL(clay_category));
								Clay__ConfigureOpenElement((Clay_ElementDeclaration){
									.layout = {
										.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
										.layoutDirection = CLAY_TOP_TO_BOTTOM,
										.padding = CLAY_PADDING_ALL(5),
									},
								});

								CLAY(CLAY_ID_LOCAL("Header"), {
									.layout = {
										.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
										.layoutDirection = CLAY_TOP_TO_BOTTOM,
										.padding.bottom = 3,
									},
									.border = {
										.color = bgame_ui_color_rgb(136, 0, 85),
										.width.bottom = 1,
									},
								}) {
									CLAY_TEXT(clay_category, {
										.fontId = FONT_CHROME,
										.fontSize = MENU_BAR_FONT_SIZE,
										.textColor = bgame_ui_color_rgb(255, 0, 160),
									});
								}
							}

							CLAY(CLAY_SID_LOCAL(((Clay_String){ .chars = &node->symbol, .length = 1 })), {
								.layout = {
									.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
									.padding = CLAY_PADDING_ALL(3),
									.childAlignment.y = CLAY_ALIGN_Y_CENTER
								},
							}) {
								CLAY(CLAY_ID_LOCAL("Symbol"), {
									.layout = {
										.sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
										.padding = CLAY_PADDING_ALL(5),
									},
								}) {
									CLAY_TEXT(((Clay_String){ .chars = &node->symbol, .length = 1 }), {
										.fontId = FONT_CHROME,
										.fontSize = 24,
										.textColor = bgame_ui_color_rgb(0, 245, 255),
									});
								}

								CLAY(CLAY_ID_LOCAL("Detail"), {
									.layout = {
										.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
										.padding = CLAY_PADDING_ALL(3),
										.layoutDirection = CLAY_TOP_TO_BOTTOM,
									},
								}) {
									CLAY_TEXT(((Clay_String){ .chars = node->title, .length = strlen(node->title) }), {
										.fontId = FONT_CHROME,
										.fontSize = 13,
										.textColor = bgame_ui_color_rgb(0, 255, 65),
									});

									CLAY_TEXT(((Clay_String){ .chars = node->description, .length = strlen(node->description) }), {
										.fontId = FONT_CHROME,
										.fontSize = 13,
										.textColor = bgame_ui_color_rgb(7, 184, 66),
									});
								}
							}
						}

						Clay__CloseElement();
					}
				}
			}
		}
		// }}}

		// Status bar {{{
		const int STATUS_BAR_FONT_SIZE = 15;
		CLAY(CLAY_ID("Bottom"), {
			.layout = {
				.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
				.padding = {
					.top = 5,
					.bottom = 7,
				},
			},
			.border = {
				.color = UI_BORDER_COLOR,
				.width = CLAY_BORDER_ALL(1),
			},
			.backgroundColor = UI_BACKGROUND_COLOR,
		}) {

#define STATUS_BOX(NAME) \
			CLAY(CLAY_ID(#NAME), { \
				.layout.padding = { .left = 5, .right = 5 }, \
			})

			STATUS_BOX(BPM) {
				bgame_ui_rich_text(CLAY_ID_LOCAL("Label"), (bgame_ui_rich_text_t){
					.text = Clay_Hovered()
						? bgame_fmt("<wave speed=%f span=5>BPM</wave>", CF_PI * pipeline_params.grid_params.bpm / 30.f)
						: "BPM",
					.text_len = -1,
					.font_id = FONT_CHROME,
					.font_size = STATUS_BAR_FONT_SIZE,
					.color = UI_TEXT_COLOR,
					.sizing = BGAME_UI_RICH_TEXT_GROW,
				});

				CLAY_TEXT(bgame_ui_string("%3d", pipeline_params.grid_params.bpm), {
					.fontId = FONT_CHROME,
					.fontSize = STATUS_BAR_FONT_SIZE,
					.textColor = bgame_ui_color_rgb(0, 245, 255),
				});

				if (Clay_Hovered()) {
					int multiplier = cf_button_binding_down(btn_scroll_multiply) ? 10 : 1;
					pipeline_params.grid_params.bpm += cf_mouse_wheel_motion() * multiplier;
					grid_modified = true;
				}
			}

			bgame_ui_hspacer(CLAY_ID_LOCAL("Spacer"));

			STATUS_BOX(State) {
				bgame_ui_rich_text(CLAY_ID_LOCAL("Label"), (bgame_ui_rich_text_t){
					.text = playing
						? bgame_fmt("<wave speed=%f>PLAYING</wave>", CF_PI * pipeline_params.grid_params.bpm / 30.f)
						: "PAUSED",
					.text_len = -1,
					.font_id = FONT_CHROME,
					.font_size = STATUS_BAR_FONT_SIZE,
					.color = UI_TEXT_COLOR,
					.sizing = BGAME_UI_RICH_TEXT_GROW,
				});
			}
		}
	}
	// }}}

	Clay_RenderCommandArray render_cmds = Clay_EndLayout(CF_DELTA_TIME);
	bgame_render_ui(render_cmds);

	// }}}

	if (grid_modified) {
		bg_pipeline_build(editor_pipeline, &node_registry, &grid);
		should_send_audio_state = true;
	}

	if (should_send_audio_state) {
		send_audio_state();
	}

	cf_app_draw_onto_screen(true);
}

BGAME_SCENE(main) = {
	.init = init,
	.update = update,
	.cleanup = cleanup,
	.before_reload = before_reload,
};
