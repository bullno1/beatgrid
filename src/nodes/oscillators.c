#include "../grid.h"
#include <cute_math.h>

#define CATEGORY "Oscillators"

typedef struct {
	float phase;
} bg_oscillator_state_t;

static void
sine_eval(bg_node_ctx_t* ctx) {
	bg_oscillator_state_t* state = bg_node_ensure_userdata(ctx, sizeof(bg_oscillator_state_t));
	if (bg_node_is_cleaning_up(ctx)) {
		bg_node_realloc(ctx, state, 0);
	}

	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t freq = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "freq",
		.description = "Frequency of the wave",
		.default_value = 440.f,  // TODO: Derive from tuning frequency
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "sig",
		.description = "Output signal",
	});

	if (bg_node_is_processing(ctx)) {
		if (!isnan(freq)) {
			bg_node_set_output_value(ctx, output, cf_sin(state->phase));

			float dt = bg_node_get_pipeline_params(ctx)->dt;
			state->phase += 2.0 * CF_PI * freq * dt;
			if (state->phase >= 2.0 * CF_PI) {
				state->phase -= 2.0 * CF_PI;
			}
		} else {
			bg_node_set_output_value(ctx, output, NAN);
		}
	}
}

BG_NODE(sine) = {
	.symbol = '~',
	.category = CATEGORY,
	.title = "Sine wave",
	.description = "Generate a sine wave with the given frequency",

	.eval = sine_eval,
};

static void
saw_eval(bg_node_ctx_t* ctx) {
	bg_oscillator_state_t* state = bg_node_ensure_userdata(ctx, sizeof(bg_oscillator_state_t));
	if (bg_node_is_cleaning_up(ctx)) {
		bg_node_realloc(ctx, state, 0);
	}

	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t freq = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "freq",
		.description = "Frequency of the wave",
		.default_value = 440.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "sig",
		.description = "Output signal",
	});

	if (bg_node_is_processing(ctx)) {
		if (!isnan(freq)) {
			bg_node_set_output_value(ctx, output, 2.0f * state->phase - 1.0f);

			float dt = bg_node_get_pipeline_params(ctx)->dt;
			state->phase += freq * dt;
			if (state->phase >= 1.0f) {
				state->phase -= 1.0f;
			}
		} else {
			bg_node_set_output_value(ctx, output, NAN);
		}
	}
}

BG_NODE(sawtooth) = {
	.symbol = 'Z',
	.category = CATEGORY,
	.title = "Sawtooth",
	.description = "Generate a sawtooth with the given frequency",

	.eval = saw_eval, };

static void
tri_eval(bg_node_ctx_t* ctx) {
	bg_oscillator_state_t* state = bg_node_ensure_userdata(ctx, sizeof(bg_oscillator_state_t));
	if (bg_node_is_cleaning_up(ctx)) {
		bg_node_realloc(ctx, state, 0);
	}

	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t freq = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "freq",
		.description = "Frequency of the wave",
		.default_value = 440.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "sig",
		.description = "Output signal",
	});

	if (bg_node_is_processing(ctx)) {
		if (!isnan(freq)) {
			float out = state->phase < 0.5f
				? 4.0f * state->phase - 1.0f
				: 3.0f - 4.0f * state->phase;
			bg_node_set_output_value(ctx, output, out);

			float dt = bg_node_get_pipeline_params(ctx)->dt;
			state->phase += freq * dt;
			if (state->phase >= 1.0f) {
				state->phase -= 1.0f;
			}
		} else {
			state->phase = 0.f;
		}
	}
}

BG_NODE(triangle) = {
	.symbol = '^',
	.category = CATEGORY,
	.title = "Triangle",
	.description = "Generate a triangle wave with the given frequency",

	.eval = tri_eval,
};

static void
sqr_eval(bg_node_ctx_t* ctx) {
	bg_oscillator_state_t* state = bg_node_ensure_userdata(ctx, sizeof(bg_oscillator_state_t));
	if (bg_node_is_cleaning_up(ctx)) {
		bg_node_realloc(ctx, state, 0);
	}

	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t freq = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "freq",
		.description = "Frequency of the wave",
		.default_value = 440.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "sig",
		.description = "Output signal",
	});

	if (bg_node_is_processing(ctx)) {
		if (!isnan(freq)) {  // TODO: duty cycle param instead of 0.5f
			bg_node_set_output_value(ctx, output, state->phase < 0.5f ? 1.0f : -1.0f);

			float dt = bg_node_get_pipeline_params(ctx)->dt;
			state->phase += freq * dt;
			if (state->phase >= 1.0f) {
				state->phase -= 1.0f;
			}
		} else {
			bg_node_set_output_value(ctx, output, NAN);
		}
	}
}

BG_NODE(square) = {
	.symbol = 'L',
	.category = CATEGORY,
	.title = "Square",
	.description = "Generate a square wave with the given frequency",

	.eval = sqr_eval,
};
