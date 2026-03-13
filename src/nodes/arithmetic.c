#include "../grid.h"

static void
eval_multiply(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t lhs = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 2, .y = pos.y },
		.name = "lhs",
		.description = "Left hand side of the operator",
		.default_value = 0.f,
	});
	bg_signal_t rhs = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "rhs",
		.description = "Right hand side of the operator",
		.default_value = 0.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "result",
		.description = "lhs * rhs",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, lhs * rhs);
	}
}

BG_NODE(multiply) = {
	.symbol = '*',
	.eval = eval_multiply,
	.title = "Multiply two numbers",
};
