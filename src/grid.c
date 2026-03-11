#include "grid.h"
#include <blog.h>
#include <nanbox.h>
#include <math.h>

AUTOLIST_DEFINE(bg_operators)

void
bg_operator_registry_reinit(bg_operator_registry_t* reg, bgame_allocator_t* allocator) {
	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	hconfig.removable = false;
	bhash_reinit(reg, hconfig);
	bhash_clear(reg);

	AUTOLIST_FOREACH(itr, bg_operators) {
		bg_operator_t* op = itr->value_addr;

		if (bhash_has(reg, op->symbol)) {
			BLOG_WARN("Symbol %c is already registered", op->symbol);
		}

		bhash_put(reg, op->symbol, op);

		BLOG_DEBUG("Registered operator '%s' as '%c'", itr->name, op->symbol);
	}
}

void
bg_operator_registry_cleanup(bg_operator_registry_t* reg) {
	bhash_cleanup(reg);
}

bg_operator_t*
bg_operator_registry_lookup(bg_operator_registry_t* reg, bg_sym_t sym) {
	bhash_index_t index = bhash_find(reg, sym);
	return bhash_is_valid(index) ? reg->values[index] : NULL;
}

bg_sig_val_t
bg_sig_val_null(void) {
	return (bg_sig_val_t){ .internal = nanbox_null().as_double };
}

bg_sig_val_t
bg_sig_val_number(double number) {
	return (bg_sig_val_t){ .internal = nanbox_from_double(number).as_double };
}

double
bg_sig_val_to_number(bg_sig_val_t sigval) {
	return bg_sig_val_to_number_with_default(sigval, NAN);
}

double
bg_sig_val_to_number_with_default(bg_sig_val_t sigval, double default_value) {
	nanbox_t nanbox = { .as_double = sigval.internal };
	return nanbox_is_double(nanbox) ? nanbox_to_double(nanbox) : default_value;
}

bool
bg_sig_val_is_null(bg_sig_val_t sigval) {
	return nanbox_is_null((nanbox_t){ .as_double = sigval.internal });
}

bool
bg_sig_val_is_number(bg_sig_val_t sigval) {
	return nanbox_is_number((nanbox_t){ .as_double = sigval.internal });
}

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
