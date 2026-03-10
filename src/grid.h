#ifndef BEATGRID_GRID_H
#define BEATGRID_GRID_H

#include <bgame/allocator.h>
#include <bhash.h>

typedef struct {
	int x;
	int y;
} bg_pos_t;

typedef char bg_sym_t;

typedef BHASH_TABLE(bg_pos_t, bg_sym_t) bg_grid_t;

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
bg_grid_put(bg_grid_t* grid, bg_pos_t pos, bg_sym_t sym) {
	if (sym == ' ') {
		bhash_remove(grid, pos);
	} else {
		bhash_put(grid, pos, sym);
	}
}

#endif
