#include "../grid.h"

#define CATEGORY "Arithmetic"

static void
eval_plus(bg_node_ctx_t* ctx) {
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
		.description = "lhs + rhs",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, lhs + rhs);
	}
}

BG_NODE(plus) = {
	.symbol = '+',
	.category = CATEGORY,
	.title = "Plus",
	.description = "Adds two inputs",

	.eval = eval_plus,
};

static void
eval_minus(bg_node_ctx_t* ctx) {
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
		.description = "lhs - rhs",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, lhs - rhs);
	}
}

BG_NODE(minus) = {
	.symbol = '-',
	.category = CATEGORY,
	.title = "Minus",
	.description = "Subtract one input from another",

	.eval = eval_minus,
};

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
	.category = CATEGORY,
	.title = "Multiply",
	.description = "Multiply two inputs",

	.eval = eval_multiply,
};

static void
eval_divide(bg_node_ctx_t* ctx) {
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
		.description = "lhs / rhs",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, lhs / rhs);
	}
}

BG_NODE(divide) = {
	.symbol = '/',
	.category = CATEGORY,
	.title = "Divide",
	.description = "Divide one input by the other",

	.eval = eval_divide,
};
