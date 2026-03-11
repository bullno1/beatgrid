#ifndef BEATGRID_GRID_H
#define BEATGRID_GRID_H

#include <bgame/allocator.h>
#include <bhash.h>
#include <autolist.h>

#define BG_OPERATOR(NAME) \
	AUTOLIST_ENTRY(bg_operators, bg_operator_t, NAME)

typedef enum {
	BG_EVAL_INIT,
	BG_EVAL_PROCESS,
	BG_EVAL_CLEANUP,
} bg_eval_phase_t;

typedef struct {
	int x;
	int y;
} bg_pos_t;

typedef struct { bg_pos_t from;
	bg_pos_t to;
} bg_edge_t;

typedef char bg_sym_t;
typedef union { double internal; } bg_sig_val_t;
typedef BHASH_TABLE(bg_pos_t, bg_sig_val_t) bg_val_grid_t;
typedef struct bg_eval_ctx_s bg_eval_ctx_t;
typedef struct bg_node_ctx_s bg_node_ctx_t;
typedef struct bg_pipeline_s bg_pipeline_t;

typedef struct {
	int bpm;
	int sample_rate;
} bg_grid_params_t;

typedef struct {
	const char* title;
	const char* description;
	bg_sym_t symbol;

	void (*eval)(bg_node_ctx_t* ctx);
} bg_operator_t;

typedef struct {
	bg_pos_t pos;
	const char* name;
	const char* description;
} bg_conn_desc_t;

typedef BHASH_TABLE(bg_pos_t, bg_sym_t) bg_grid_t;
typedef BHASH_TABLE(bg_sym_t, bg_operator_t*) bg_operator_registry_t;

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

void
bg_operator_registry_reinit(bg_operator_registry_t* reg, bgame_allocator_t* allocator);

void
bg_operator_registry_cleanup(bg_operator_registry_t* reg);

bg_operator_t*
bg_operator_registry_lookup(bg_operator_registry_t* reg, bg_sym_t sym);

bg_sig_val_t
bg_sig_val_null(void);

bg_sig_val_t
bg_sig_val_number(double number);

bool
bg_sig_val_is_null(bg_sig_val_t sigval);

bool
bg_sig_val_is_number(bg_sig_val_t sigval);

double
bg_sig_val_to_number(bg_sig_val_t sigval);

double
bg_sig_val_to_number_with_default(bg_sig_val_t sigval, double default_value);

void
bg_pipeline_reinit(bg_pipeline_t** pipeline, bgame_allocator_t* allocator);

void
bg_pipeline_cleanup(bg_pipeline_t** pipeline);

void
bg_pipeline_build(bg_pipeline_t* pipeline, bg_grid_t* grid);

bg_eval_ctx_t*
bg_pipeline_begin_eval(bg_pipeline_t* pipeline, bg_grid_params_t params);

void
bg_pipeline_end_eval(bg_eval_ctx_t* ctx);

void
bg_pipeline_step(bg_eval_ctx_t* ctx);

int
bg_pipeline_count_edges(bg_eval_ctx_t* ctx);

const bg_edge_t*
bg_pipeline_get_edges(bg_eval_ctx_t* ctx);

bg_grid_params_t
bg_node_get_grid_params(bg_node_ctx_t* ctx);

bg_pos_t
bg_node_get_pos(bg_node_ctx_t* ctx);

bg_eval_phase_t
bg_node_get_eval_phase(bg_node_ctx_t* ctx);

bg_sig_val_t
bg_node_get_input(bg_node_ctx_t* ctx, const bg_conn_desc_t* desc);

bg_sig_val_t*
bg_node_get_output(bg_node_ctx_t* ctx, const bg_conn_desc_t* desc);

float
bg_node_get_phase_accumulator(bg_node_ctx_t* ctx);

void
bg_node_set_phase_accumulator(bg_node_ctx_t* ctx, double value);

#endif
