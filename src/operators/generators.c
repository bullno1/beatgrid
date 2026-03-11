#include "../grid.h"
#include <cute_math.h>

static void
sine_eval(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_sig_val_t input = bg_node_get_input(ctx, &(bg_conn_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "freq",
		.description = "Frequency of the wave",
	});
	bg_sig_val_t* output = bg_node_get_output(ctx, &(bg_conn_desc_t){
		.pos = { .x = pos.x, .y = pos.y + 1 },
		.name = "sig",
		.description = "Output signal",
	});

	if (bg_node_get_eval_phase(ctx) == BG_EVAL_PROCESS) {
		float phase = bg_node_get_phase_accumulator(ctx);
		*output = bg_sig_val_number(cf_sin(phase * 2.f * CF_PI));

		float freq = bg_sig_val_to_number_with_default(input, 440.f);
		bg_grid_params_t params = bg_node_get_grid_params(ctx);
		phase += freq / (float)params.sample_rate;
		if (phase >= 1.f) { phase -= 1.f; }
		bg_node_set_phase_accumulator(ctx, phase);
	}
}

BG_OPERATOR(sine) = {
	.symbol = 'S',
	.eval = sine_eval,
	.title = "Sine wave generator",
};
