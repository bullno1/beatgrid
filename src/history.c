#include "history.h"

void
history_reinit(history_t** history_ptr, struct bgame_allocator_s* allocator, int num_entries) {
	history_t* history = *history_ptr;

	size_t size = sizeof(history_t) + sizeof(history->entries[0]) * num_entries;
	history = bgame_realloc(history, size, allocator);
	if (*history_ptr == NULL) {
		memset(history, 0, size);
	}

	for (int i = 0; i < num_entries; ++i) {
		bg_grid_reinit(&history->entries[i].grid, allocator);
	}

	history->num_entries = num_entries;
	*history_ptr = history;
}

void
history_cleanup(history_t** history_ptr, struct bgame_allocator_s* allocator) {
	history_t* history = *history_ptr;
	if (history == NULL) { return; }

	for (int i = 0; i < history->num_entries; ++i) {
		bg_grid_cleanup(&history->entries[i].grid);
	}

	bgame_free(history, allocator);

	*history_ptr = NULL;
}

const bg_grid_t*
history_current_copy(history_t* history) {
	return &history->entries[history->current_index].grid;
}

history_version_t
history_current_version(history_t* history) {
	return history->entries[history->current_index].version;
}

bg_grid_t*
history_begin_edit(history_t* history) {
	history_entry_t* current_entry = &history->entries[history->current_index];
	int next_index = (history->current_index + 1) % history->num_entries;
	history_entry_t* next_entry = &history->entries[next_index];

	bg_grid_copy(&next_entry->grid, &current_entry->grid);
	next_entry->version = ++history->current_version;
	history->current_index = next_index;
	return &next_entry->grid;
}

void
history_end_edit(history_t* history) {
	// TODO: set and unset a status flag
}

const bg_grid_t*
history_undo(history_t* history) {
	int prev_index = history->current_index - 1;
	if (prev_index < 0) { prev_index += history->num_entries; }
	history_entry_t* prev_entry = &history->entries[prev_index];
	if (prev_entry->version < history->entries[history->current_index].version) {
		history->current_index = prev_index;
	}

	return history_current_copy(history);
}

const bg_grid_t*
history_redo(history_t* history) {
	int next_index = (history->current_index + 1) % history->num_entries;
	history_entry_t* next_entry = &history->entries[next_index];
	if (next_entry->version > history->entries[history->current_index].version) {
		history->current_index = next_index;
	}

	return history_current_copy(history);
}

bool
history_can_undo(history_t* history) {
	int prev_index = history->current_index - 1;
	if (prev_index < 0) { prev_index += history->num_entries; }
	history_entry_t* prev_entry = &history->entries[prev_index];
	return prev_entry->version < history->entries[history->current_index].version;
}

bool
history_can_redo(history_t* history) {
	int next_index = (history->current_index + 1) % history->num_entries;
	history_entry_t* next_entry = &history->entries[next_index];
	return next_entry->version > history->entries[history->current_index].version;
}

void
history_clear(history_t* history) {
	history->current_index = 0;
	history->current_version += 1;

	for (int i = 0; i < history->num_entries; ++i) {
		history->entries[i].version = history->current_version;
	}

	history_entry_t* entry = &history->entries[history->current_index];

	bg_grid_clear(&entry->grid);
	entry->version = history->current_version;
}
