#ifndef BEATGRID_HISTORY_H
#define BEATGRID_HISTORY_H

#include "grid.h"
#include <stdint.h>
#include <stdbool.h>

struct bgame_allocator_s;
typedef uint64_t history_version_t;

typedef struct {
	bg_grid_t grid;
	history_version_t version;
} history_entry_t;

typedef struct {
	int num_entries;
	int current_index;
	history_version_t current_version;
	history_entry_t entries[];
} history_t;

void
history_reinit(history_t** history_ptr, struct bgame_allocator_s* allocator, int num_entries);

void
history_cleanup(history_t** history_ptr, struct bgame_allocator_s* allocator);

history_version_t
history_current_version(history_t* history);

const bg_grid_t*
history_current_copy(history_t* history);

bg_grid_t*
history_begin_edit(history_t* history);

void
history_end_edit(history_t* history);

const bg_grid_t*
history_undo(history_t* history);

const bg_grid_t*
history_redo(history_t* history);

bool
history_can_undo(history_t* history);

bool
history_can_redo(history_t* history);

void
history_clear(history_t* history);

#define EDIT_GRID(HISTORY, GRID) \
	for ( \
		bg_grid_t* GRID = history_begin_edit(HISTORY); \
		GRID != NULL; \
		GRID = NULL, history_end_edit(HISTORY) \
	)

#endif
