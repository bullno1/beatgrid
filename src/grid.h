#ifndef BEATGRID_GRID_H
#define BEATGRID_GRID_H

#include <bgame/allocator.h>
#include <bhash.h>
#include <autolist.h>

#define BG_NODE(NAME) \
	AUTOLIST_ENTRY(bg_nodes, bg_node_t, NAME)

typedef enum {
	BG_EVAL_INIT,
	BG_EVAL_PROBE,
	BG_EVAL_PROCESS,
	BG_EVAL_CLEANUP,
} bg_eval_phase_t;

typedef struct {
	int x;
	int y;
} bg_pos_t;

typedef struct {
	bg_pos_t from;
	bg_pos_t to;
} bg_edge_t;

typedef char bg_sym_t;
typedef float bg_signal_t;
typedef struct bg_eval_ctx_s bg_eval_ctx_t;
typedef struct bg_node_ctx_s bg_node_ctx_t;
typedef struct bg_pipeline_s bg_pipeline_t;
typedef struct { bg_pos_t pos; bool enabled; } bg_output_handle_t;

typedef struct {
	int count;
	float value;
} bg_output_t;

typedef struct {
	const char* category;
	const char* title;
	const char* description;
	bg_sym_t symbol;
	bool pull;

	void (*eval)(bg_node_ctx_t* ctx);
} bg_node_t;

typedef struct {
	bg_pos_t pos;
	const char* name;
	const char* description;
	bg_signal_t default_value;
	bool* input_found;
} bg_input_desc_t;

typedef struct {
	bg_pos_t pos;
	const char* name;
	const char* description;
} bg_output_desc_t;

typedef struct {
	bool is_input;

	union {
		bg_input_desc_t input;
		bg_output_desc_t output;
	} desc;
} bg_edge_data_t;

typedef BHASH_TABLE(bg_pos_t, bg_sym_t) bg_grid_t;
typedef BHASH_TABLE(bg_sym_t, const bg_node_t*) bg_node_registry_t;

typedef struct {
	int bpm;
} bg_grid_params_t;

typedef struct {
	bg_grid_params_t grid_params;
	float dt;

	int num_inputs;
	float* inputs;

	int num_outputs;
	bg_output_t* outputs;
} bg_pipeline_params_t;

static inline void
bg_grid_reinit(bg_grid_t* grid, bgame_allocator_t* allocator) {
	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	bhash_reinit(grid, hconfig);
}

static inline void
bg_grid_cleanup(bg_grid_t* grid) {
	bhash_cleanup(grid);
}

static inline void
bg_grid_del(bg_grid_t* grid, bg_pos_t pos) {
	bhash_remove(grid, pos);
}

static inline void
bg_grid_put(bg_grid_t* grid, bg_pos_t pos, bg_sym_t sym) {
	if (sym == ' ') {
		bg_grid_del(grid, pos);
	} else {
		bhash_put(grid, pos, sym);
	}
}

static inline void
bg_grid_copy(bg_grid_t* dst, const bg_grid_t* src) {
	bhash_clear(dst);
	bhash_index_t len = bhash_len(src);
	for (bhash_index_t i = 0; i < len; ++i) {
		bhash_put(dst, src->keys[i], src->values[i]);
	}
}

static inline void
bg_grid_clear(bg_grid_t* grid) {
	bhash_clear(grid);
}

void
bg_node_registry_reinit(bg_node_registry_t* reg, bgame_allocator_t* allocator);

void
bg_node_registry_cleanup(bg_node_registry_t* reg);

const bg_node_t*
bg_node_registry_lookup(const bg_node_registry_t* reg, bg_sym_t sym);

void
bg_pipeline_reinit(bg_pipeline_t** pipeline, bgame_allocator_t* allocator);

void
bg_pipeline_cleanup(bg_pipeline_t** pipeline);

void
bg_pipeline_set_params(bg_pipeline_t* pipeline, bg_pipeline_params_t params);

void
bg_pipeline_build(
	bg_pipeline_t* pipeline,
	const bg_node_registry_t* node_registry,
	const bg_grid_t* grid
);

int
bg_pipeline_count_edges(bg_pipeline_t* pipeline);

const bg_edge_t*
bg_pipeline_get_edges(bg_pipeline_t* pipeline);

const bg_edge_data_t*
bg_pipeline_get_edge_data(bg_pipeline_t* pipeline);

void
bg_pipeline_process(bg_pipeline_t* pipeline);

const bg_pipeline_params_t*
bg_node_get_pipeline_params(bg_node_ctx_t* ctx);

void*
bg_node_get_userdata(bg_node_ctx_t* ctx);

void
bg_node_set_userdata(bg_node_ctx_t* ctx, void* userdata);

void*
bg_node_realloc(bg_node_ctx_t* ctx, void* ptr, size_t size);

bg_pos_t
bg_node_get_pos(bg_node_ctx_t* ctx);

bg_eval_phase_t
bg_node_get_eval_phase(bg_node_ctx_t* ctx);

bg_signal_t
bg_node_get_input(bg_node_ctx_t* ctx, bg_input_desc_t desc);

bg_output_handle_t
bg_node_get_output(bg_node_ctx_t* ctx, bg_output_desc_t desc);

void
bg_node_set_output_value(
	bg_node_ctx_t* ctx,
	bg_output_handle_t output,
	bg_signal_t value
);

void
bg_node_push_pipeline_output(
	bg_node_ctx_t* ctx,
	int channel,
	bg_signal_t value
);

static bool
bg_node_is_initializing(bg_node_ctx_t* ctx) {
	return bg_node_get_eval_phase(ctx) == BG_EVAL_INIT;
}

static bool
bg_node_is_cleaning_up(bg_node_ctx_t* ctx) {
	return bg_node_get_eval_phase(ctx) == BG_EVAL_CLEANUP;
}

static bool
bg_node_is_processing(bg_node_ctx_t* ctx) {
	return bg_node_get_eval_phase(ctx) == BG_EVAL_PROCESS;
}

static void*
bg_node_ensure_userdata(bg_node_ctx_t* ctx, size_t size) {
	void* userdata = bg_node_get_userdata(ctx);

	if (userdata == NULL) {
		userdata = bg_node_realloc(ctx, NULL, size);
		bg_node_set_userdata(ctx, userdata);
		memset(userdata, 0, size);
	}

	return userdata;
}

#endif
