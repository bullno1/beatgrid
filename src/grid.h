#ifndef GRIDBEAT_GRID_H
#define GRIDBEAT_GRID_H

#include <bgame/allocator.h>
#include <bhash.h>

typedef struct {
	int x;
	int y;
} gb_pos_t;

typedef char gb_sym_t;

typedef BHASH_TABLE(gb_pos_t, gb_sym_t) gb_grid_t;

static inline void
gb_grid_reinit(gb_grid_t* grid, bgame_allocator_t* allocator) {
	bhash_config_t hconfig = bhash_config_default();
	hconfig.memctx = allocator;
	bhash_reinit(grid, hconfig);
}

static inline void
gb_grid_cleanup(gb_grid_t* grid) {
	bhash_cleanup(grid);
}

static inline void
gb_grid_put(gb_grid_t* grid, gb_pos_t pos, gb_sym_t sym) {
	if (sym == ' ') {
		bhash_remove(grid, pos);
	} else {
		bhash_put(grid, pos, sym);
	}
}

#endif
