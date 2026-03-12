#include "../grid.h"

static void
output_eval(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t output = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "o",
		.description = "Output signal",
		.default_value = 0.f,
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_push_pipeline_output(ctx, 0, output);
	}
}

BG_NODE(output) = {
	.symbol = 'O',
	.eval = output_eval,
	.title = "Output",
	.pull = true,
};
