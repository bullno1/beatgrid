#include "../grid.h"
#include <cute_math.h>

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

	if (!isnan(freq) && bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, cf_sin(state->phase * 2.f * CF_PI));

		state->phase += freq * bg_node_get_pipeline_params(ctx)->dt;
		if (state->phase >= 1.f) { state->phase -= 1.f; }
	}
}

BG_NODE(sine) = {
	.symbol = '~',
	.eval = sine_eval,
	.title = "Sine wave generator",
};
