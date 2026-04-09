#include "../grid.h"

#define CATEGORY "Transport"

static void
eval_north(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t input = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x, .y = pos.y - 1 },
		.name = "i",
		.description = "Input value",
		.default_value = 0.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x, .y = pos.y + 1 },
		.name = "o",
		.description = "Output value",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, input);
	}
}

BG_NODE(north) = {
	.symbol = 'N',
	.category = CATEGORY,
	.title = "North",
	.description = "Transport a value northward",

	.eval = eval_north,
};

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
	.category = CATEGORY,
	.title = "South",
	.description = "Transport a value southward",

	.eval = eval_south,
};

static void
eval_east(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t input = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "i",
		.description = "Input value",
		.default_value = 0.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x + 1, .y = pos.y },
		.name = "o",
		.description = "Output value",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, input);
	}
}

BG_NODE(east) = {
	.symbol = 'E',
	.category = CATEGORY,
	.title = "East",
	.description = "Transport a value eastward",

	.eval = eval_east,
};

static void
eval_west(bg_node_ctx_t* ctx) {
	bg_pos_t pos = bg_node_get_pos(ctx);

	bg_signal_t input = bg_node_get_input(ctx, (bg_input_desc_t){
		.pos = { .x = pos.x + 1, .y = pos.y },
		.name = "i",
		.description = "Input value",
		.default_value = 0.f,
	});
	bg_output_handle_t output = bg_node_get_output(ctx, (bg_output_desc_t){
		.pos = { .x = pos.x - 1, .y = pos.y },
		.name = "o",
		.description = "Output value",
	});

	if (bg_node_is_processing(ctx)) {
		bg_node_set_output_value(ctx, output, input);
	}
}

BG_NODE(west) = {
	.symbol = 'W',
	.category = CATEGORY,
	.title = "West",
	.description = "Transport a value westward",

	.eval = eval_west,
};
