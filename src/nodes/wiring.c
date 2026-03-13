#include "../grid.h"

static void
eval_south(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t input = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x, .y = pos.y + 1 },
		.name = "i",
		.description = "Input value",
		.default_value = 0.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "o",
		.description = "Output value",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, input);
	}
}

BG_NODE(south) = {
	.symbol = 'S',
	.eval = eval_south,
	.title = "South",
	.description = "Transport a value southward"
};
