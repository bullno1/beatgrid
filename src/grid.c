#include "grid.h"
#include <blog.h>
#include <barray.h>

typedef struct {
	void* userdata;
	const bg_node_t* node;
	int version;
	bg_sym_t symbol;
} bg_node_data_t;

typedef struct {
	bg_pos_t pos;
	bg_signal_t value;
} bg_const_t;

typedef struct {
	bg_pos_t pos;
	bg_node_data_t* node_data;
} bg_pull_arc_t;

struct bg_pipeline_s {
	bgame_allocator_t* allocator;
	bg_pipeline_params_t params;
	int version;

	BHASH_TABLE(bg_pos_t, bg_node_data_t) nodes;
	BHASH_TABLE(bg_pos_t, bg_signal_t) consts;
	BHASH_TABLE(bg_pos_t, bg_signal_t) values;
	BHASH_TABLE(bg_pos_t, bg_pull_arc_t) pull_arcs;
	BHASH_TABLE(bg_edge_t, bg_edge_data_t) edges;
	barray(bg_pos_t) pull_nodes;
};

struct bg_node_ctx_s {
	bg_eval_phase_t phase;
	bg_pipeline_t* pipeline;
	bg_node_data_t* node_data;
	bg_pos_t pos;
};

AUTOLIST_DEFINE(bg_nodes)

static int
bg_decode_base36(char c) {
    if ('0' <= c && c <= '9') {
		return c - '0';
	} else if ('a' <= c && c <= 'z') {
		return c - 'a' + 10;
	} else {
		return -1;  // Invalid
	}
}

// https://nullprogram.com/blog/2018/07/31/
static inline uint64_t
splittable64(uint64_t x) {
	x ^= x >> 30;
	x *= 0xbf58476d1ce4e5b9U;
	x ^= x >> 27;
	x *= 0x94d049bb133111ebU;
	x ^= x >> 31;
	return x;
}

static bhash_hash_t
bg_hash_pos(const void* key, size_t size) {
	bg_pos_t pos = *(bg_pos_t*)key;
	return splittable64((uint64_t)pos.x << 32 | pos.y);
}

static bhash_hash_t
bg_hash_edge(const void* key, size_t size) {
	return bhash_hash(key, sizeof(bg_edge_t));
}

void
bg_node_registry_reinit(bg_node_registry_t* reg, bgame_allocator_t* allocator) {
	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	hconfig.removable = false;
	bhash_reinit(reg, hconfig);
	bhash_clear(reg);

	AUTOLIST_FOREACH(itr, bg_nodes) {
		const bg_node_t* op = itr->value_addr;
		bg_sym_t symbol = op->symbol;

		if (bhash_has(reg, symbol)) {
			BLOG_WARN("Symbol %c is already registered", op->symbol);
		}

		bhash_put(reg, symbol, op);

		BLOG_DEBUG("Registered node '%s' as '%c'", itr->name, op->symbol);
	}
}

void
bg_node_registry_cleanup(bg_node_registry_t* reg) {
	bhash_cleanup(reg);
}

const bg_node_t*
bg_node_registry_lookup(const bg_node_registry_t* reg, bg_sym_t sym) {
	bhash_index_t index = bhash_find(reg, sym);
	return bhash_is_valid(index) ? reg->values[index] : NULL;
}

void
bg_pipeline_reinit(bg_pipeline_t** pipeline_ptr, bgame_allocator_t* allocator) {
	bg_pipeline_t* pipeline = *pipeline_ptr;
	if (pipeline == NULL) {
		pipeline = bgame_malloc(sizeof(*pipeline), allocator);
		*pipeline = (bg_pipeline_t){
			.allocator = allocator,
		};
		*pipeline_ptr = pipeline;
	}

	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	hconfig.hash = bg_hash_pos;

	bhash_reinit(&pipeline->nodes, hconfig);
	bhash_reinit(&pipeline->pull_arcs, hconfig);

	hconfig.removable = false;
	bhash_reinit(&pipeline->consts, hconfig);
	bhash_reinit(&pipeline->values, hconfig);
	hconfig.hash = bg_hash_edge;
	bhash_reinit(&pipeline->edges, hconfig);
}

void
bg_pipeline_cleanup(bg_pipeline_t** pipeline_ptr) {
	bg_pipeline_t* pipeline = *pipeline_ptr;

	bhash_cleanup(&pipeline->nodes);
	bhash_cleanup(&pipeline->consts);
	bhash_cleanup(&pipeline->values);
	bhash_cleanup(&pipeline->edges);
	bhash_cleanup(&pipeline->pull_arcs);
	barray_free(pipeline->pull_nodes, pipeline->allocator);
	bgame_free(pipeline, pipeline->allocator);

	*pipeline_ptr = NULL;
}

void
bg_pipeline_set_params(bg_pipeline_t* pipeline, bg_pipeline_params_t params) {
	pipeline->params = params;
}

void
bg_pipeline_build(
	bg_pipeline_t* pipeline,
	const bg_node_registry_t* node_registry,
	const bg_grid_t* grid
) {
	bhash_clear(&pipeline->consts);
	barray_clear(pipeline->pull_nodes);
	int version = ++pipeline->version;

	// Build pipeline from the grid
	for (bhash_index_t i = 0; i < bhash_len(grid); ++i) {
		bg_pos_t pos = grid->keys[i];
		char symbol = grid->values[i];

		int num = bg_decode_base36(symbol);
		if (num >= 0) {
			bhash_put(&pipeline->consts, pos, (bg_signal_t){ num });
		} else {
			const bg_node_t* node = bg_node_registry_lookup(node_registry, symbol);

			if (node == NULL) { continue; }

			bhash_alloc_result_t alloc_result = bhash_alloc(&pipeline->nodes, pos);
			pipeline->nodes.keys[alloc_result.index] = pos;
			bg_node_data_t* node_data = &pipeline->nodes.values[alloc_result.index];

			bool need_init = true;
			void* userdata = NULL;
			if (!alloc_result.is_new) {
				if (node_data->symbol == symbol) {
					// Same type, propagate userdata which could still be NULL
					// so we need the need_init flag to control reinitialization
					need_init = false;
					userdata = node_data->userdata;
				} else {
					// Type change, cleanup existing userdata
					const bg_node_t* old_node = bg_node_registry_lookup(node_registry, node_data->symbol);
					bg_node_ctx_t ctx = {
						.phase = BG_EVAL_CLEANUP,
						.pipeline = pipeline,
						.pos = pos,
						.node_data = node_data,
					};
					old_node->eval(&ctx);
				}
			}

			*node_data = (bg_node_data_t){
				.node = node,
				.symbol = symbol,
				.version = version,
				.userdata = userdata,
			};

			if (need_init) {
				bg_node_ctx_t ctx = {
					.phase = BG_EVAL_INIT,
					.pipeline = pipeline,
					.pos = pos,
					.node_data = node_data,
				};
				node_data->node->eval(&ctx);
			}

			if (node->pull) {
				barray_push(pipeline->pull_nodes, pos, pipeline->allocator);
			}
		}
	}

	// Clean up removed nodes
	for (bhash_index_t i = 0; i < bhash_len(&pipeline->nodes);) {
		bg_pos_t pos = pipeline->nodes.keys[i];
		bg_node_data_t* node_data = &pipeline->nodes.values[i];

		if (node_data->version == version) { ++i; continue; }

		// Lookup again instead of relying on node_data->node since there could
		// be a reload
		const bg_node_t* node = bg_node_registry_lookup(node_registry, node_data->symbol);
		if (node == NULL) {
			BLOG_WARN(
				"Could not find node for symbol '%c' at (%d, %d)",
				node_data->symbol,
				pos.x, pos.y
			);
			continue;
		}

		bg_node_ctx_t ctx = {
			.phase = BG_EVAL_CLEANUP,
			.pipeline = pipeline,
			.pos = pos,
			.node_data = node_data,
		};
		node->eval(&ctx);

		bhash_remove(&pipeline->nodes, pos);
	}

	// Write consts
	bhash_clear(&pipeline->values);
	for (bhash_index_t i = 0; i < bhash_len(&pipeline->consts); ++i) {
		bhash_put(&pipeline->values, pipeline->consts.keys[i], pipeline->consts.values[i]);
	}

	// Probe for connections
	bhash_clear(&pipeline->edges);
	bhash_clear(&pipeline->pull_arcs);
	for (int probe_count = 0; probe_count < 2 ; ++probe_count) {
		// Run probe twice so that input/output order does not matter
		for (bhash_index_t i = 0; i < bhash_len(&pipeline->nodes); ++i) {
			bg_pos_t pos = pipeline->nodes.keys[i];
			bg_node_data_t* node_data = &pipeline->nodes.values[i];
			bg_node_ctx_t ctx = {
				.phase = BG_EVAL_PROBE,
				.pipeline = pipeline,
				.pos = pos,
				.node_data = node_data,
			};
			node_data->node->eval(&ctx);
		}
	}
}

int
bg_pipeline_count_edges(bg_pipeline_t* pipeline);

const bg_edge_t*
bg_pipeline_get_edges(bg_pipeline_t* pipeline);

const bg_pipeline_params_t*
bg_node_get_pipeline_params(bg_node_ctx_t* ctx) {
	return &ctx->pipeline->params;
}

void*
bg_node_get_userdata(bg_node_ctx_t* ctx) {
	return ctx->node_data->userdata;
}

void
bg_node_set_userdata(bg_node_ctx_t* ctx, void* userdata) {
	ctx->node_data->userdata = userdata;
}

void*
bg_node_realloc(bg_node_ctx_t* ctx, void* ptr, size_t size) {
	return bgame_realloc(ptr, size, ctx->pipeline->allocator);
}

bg_pos_t
bg_node_get_pos(bg_node_ctx_t* ctx) {
	return ctx->pos;
}

bg_eval_phase_t
bg_node_get_eval_phase(bg_node_ctx_t* ctx) {
	return ctx->phase;
}

bg_signal_t
bg_node_get_input(bg_node_ctx_t* ctx, bg_input_desc_t desc) {
	bg_signal_t* value = bhash_get_value(&ctx->pipeline->values, desc.pos);

	if (ctx->phase == BG_EVAL_PROBE) {
		bg_edge_t edge = {
			.from = desc.pos,
			.to = ctx->pos,
		};
		bg_edge_data_t edge_data = {
			.is_input = true,
			.desc.input = desc,
		};
		edge_data.desc.input.input_found = NULL;
		bhash_put(&ctx->pipeline->edges, edge, edge_data);
	}

	if (value == NULL && ctx->phase == BG_EVAL_PROCESS) {
		// Pull data from dependency
		bg_pull_arc_t* arc = bhash_get_value(&ctx->pipeline->pull_arcs, desc.pos);
		if (arc != NULL) {
			bg_pull_arc_t arc_val = *arc;  // Make a copy since we are going to remove this
			bg_node_ctx_t inner_ctx = {
				.phase = BG_EVAL_PROCESS,
				.pipeline = ctx->pipeline,
				.node_data = arc_val.node_data,
				.pos = arc_val.pos,
			};
			bhash_remove(&ctx->pipeline->pull_arcs, desc.pos);  // Prevent cycle
			arc_val.node_data->node->eval(&inner_ctx);
			value = bhash_get_value(&ctx->pipeline->values, desc.pos);
		}
	}

	if (desc.input_found != NULL) {
		*desc.input_found = value != NULL;
	}

	return value != NULL ? *value : desc.default_value;
}


bg_output_handle_t
bg_node_get_output(bg_node_ctx_t* ctx, bg_output_desc_t desc) {
	if (ctx->phase == BG_EVAL_PROBE) {
		bg_edge_t edge = {
			.from = desc.pos,
			.to = ctx->pos,
		};
		bg_edge_data_t edge_data = {
			.is_input = false,
			.desc.output = desc,
		};
		edge_data.desc.input.input_found = NULL;
		bhash_put(&ctx->pipeline->edges, edge, edge_data);

		bhash_alloc_result_t alloc_result = bhash_alloc(&ctx->pipeline->pull_arcs, desc.pos);
		if (alloc_result.is_new) {
			ctx->pipeline->pull_arcs.keys[alloc_result.index] = desc.pos;
			ctx->pipeline->pull_arcs.values[alloc_result.index] = (bg_pull_arc_t){
				.pos = ctx->pos,
				.node_data = ctx->node_data,
			};

			return (bg_output_handle_t){
				.enabled = true,
				.pos = desc.pos,
			};
		} else {
			// TODO: highlight conflicting outputs (2 nodes writes to the same cell)
			return (bg_output_handle_t){ .enabled = false };
		}
	} else {
		return (bg_output_handle_t){
			.enabled = true,
			.pos = desc.pos,
		};
	}
}

void
bg_node_set_output_value(bg_node_ctx_t* ctx, bg_output_handle_t handle, bg_signal_t value) {
	if (handle.enabled && ctx->phase == BG_EVAL_PROCESS) {
		bhash_put(&ctx->pipeline->values, handle.pos, value);
	}
}

int
bg_pipeline_count_edges(bg_pipeline_t* pipeline) {
	return bhash_len(&pipeline->edges);
}

const bg_edge_t*
bg_pipeline_get_edges(bg_pipeline_t* pipeline) {
	return pipeline->edges.keys;
}

const bg_edge_data_t*
bg_pipeline_get_edge_data(bg_pipeline_t* pipeline) {
	return pipeline->edges.values;
}

void
bg_pipeline_process(bg_pipeline_t* pipeline) {
	// Reset output
	for (int i = 0; i < pipeline->params.num_outputs; ++i) {
		pipeline->params.outputs[i].count = 0;
		pipeline->params.outputs[i].value = 0.f;
	}

	// Reset grid
	bhash_clear(&pipeline->values);
	for (bhash_index_t i = 0; i < bhash_len(&pipeline->consts); ++i) {
		bhash_put(&pipeline->values, pipeline->consts.keys[i], pipeline->consts.values[i]);
	}

	// Prepare pull arcs
	bhash_clear(&pipeline->pull_arcs);
	for (int probe_count = 0; probe_count < 2 ; ++probe_count) {
		for (bhash_index_t i = 0; i < bhash_len(&pipeline->nodes); ++i) {
			bg_pos_t pos = pipeline->nodes.keys[i];
			bg_node_data_t* node_data = &pipeline->nodes.values[i];
			bg_node_ctx_t ctx = {
				.phase = BG_EVAL_PROBE,
				.pipeline = pipeline,
				.pos = pos,
				.node_data = node_data,
			};
			node_data->node->eval(&ctx);
		}
	}

	// Evaluate from pull nodes
	BARRAY_FOREACH_VALUE(pos, pipeline->pull_nodes) {
		bg_node_data_t* node_data = bhash_get_value(&pipeline->nodes, pos);
		if (node_data == NULL) { continue; }  // Should not be possible

		bg_node_ctx_t ctx = {
			.phase = BG_EVAL_PROCESS,
			.pipeline = pipeline,
			.pos = pos,
			.node_data = node_data,
		};
		node_data->node->eval(&ctx);
	}
}

void
bg_node_push_pipeline_output(
	bg_node_ctx_t* ctx,
	int channel,
	bg_signal_t value
) {
	if (0 <= channel && channel < ctx->pipeline->params.num_outputs) {
		ctx->pipeline->params.outputs[channel].count += 1;
		ctx->pipeline->params.outputs[channel].value += value;
	}
}
