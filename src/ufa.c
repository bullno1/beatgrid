// vim: set foldmethod=marker foldlevel=0:
#include "ufa.h"

#ifndef __EMSCRIPTEN__

// Common {{{

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_iostream.h>
#include <cute_app.h>

_Static_assert(sizeof(SDL_DialogFileFilter) == sizeof(ufa_filter_t), "SDL_DialogFileFilter has a different size");
_Static_assert(offsetof(SDL_DialogFileFilter, name) == offsetof(ufa_filter_t, name), "SDL_DialogFileFilter.name has a different offset");
_Static_assert(offsetof(SDL_DialogFileFilter, pattern) == offsetof(ufa_filter_t, pattern), "SDL_DialogFileFilter.pattern has a different offset");

typedef struct {
	barena_t* arena;
	const ufa_filter_t* filters;
	const char* filename;
	ufa_status_t status;
	SDL_IOStream* stream;
	const char* mode;
} ufa_t;

static void
ufa_open_file(ufa_t* ufa, const char* filename) {
	size_t len = strlen(filename);
	char* filename_copy = barena_memalign(ufa->arena, len + 1, _Alignof(char));
	memcpy(filename_copy, filename, len + 1);
	ufa->filename = filename_copy;

	ufa->stream = SDL_IOFromFile(filename, ufa->mode);
	ufa->status = ufa->stream != NULL ? UFA_OK : UFA_ERROR;
}

static bool
ufa_str_ends_with(const char *str, const char *suffix) {
	if (!str || !suffix) { return false; }

	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);

	if (suffix_len > str_len) { return false; }

	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static void SDLCALL
ufa_file_callback(void* userdata, const char* const* filelist, int filter) {
	ufa_t* ufa = userdata;

	if (filelist == NULL) {
		ufa->status = UFA_ERROR;
	} else if (*filelist == NULL) {
		ufa->status = UFA_CANCELLED;
	} else {
		const char* filename = *filelist;
		const char* extension = ufa->filters[filter >= 0 ? filter : 0].pattern;
		if (ufa->mode[0] == 'w' && !ufa_str_ends_with(filename, extension)) {
			size_t name_buf_len = strlen(filename) + strlen(extension) + 2;
			char* name_buf = barena_memalign(ufa->arena, name_buf_len, _Alignof(char));
			snprintf(name_buf, name_buf_len, "%s.%s", filename, extension);
			filename = name_buf;
		}

		ufa_open_file(ufa, filename);
	}
}

// }}}

// Open {{{

ufa_open_file_t*
ufa_begin_open_file(ufa_config_t config) {
	ufa_t* ufa = barena_memalign(config.arena, sizeof(ufa_t), _Alignof(ufa_t));
	*ufa = (ufa_t){
		.arena = config.arena,
		.filters = config.filters,
		.status = UFA_PENDING,
		.mode = "r",
	};

	SDL_ShowOpenFileDialog(
		ufa_file_callback, ufa,
		cf_app_get_window(),
		(const SDL_DialogFileFilter*)config.filters,
		config.num_filters,
		config.directory,
		false
	);

	return (ufa_open_file_t*)ufa;
}

ufa_status_t
ufa_check_open_file(ufa_open_file_t* open_file) {
	return ((ufa_t*)open_file)->status;
}

const char*
ufa_get_open_file_error(ufa_open_file_t* open_file) {
	return SDL_GetError();
}

const char*
ufa_get_open_file_name(ufa_open_file_t* open_file) {
	return ((ufa_t*)open_file)->filename;
}

ufa_status_t
ufa_read_open_file(ufa_open_file_t* open_file, void* buf, size_t* size) {
	ufa_t* ufa = (ufa_t*)open_file;
	if (ufa->stream == NULL) {
		*size = 0;
		return UFA_ERROR;
	}

	*size = SDL_ReadIO(ufa->stream, buf, *size);
	SDL_IOStatus status = SDL_GetIOStatus(ufa->stream);
	return status == SDL_IO_STATUS_READY || status == SDL_IO_STATUS_EOF ? UFA_OK : UFA_ERROR;
}

void
ufa_end_open_file(ufa_open_file_t* open_file) {
	ufa_t* ufa = (ufa_t*)open_file;
	if (ufa->stream) {
		SDL_CloseIO(ufa->stream);
		ufa->stream = NULL;
	}
}

// }}}

// Save {{{

ufa_save_file_t*
ufa_begin_save_file(ufa_config_t config) {
	ufa_t* ufa = barena_memalign(config.arena, sizeof(ufa_t), _Alignof(ufa_t));
	*ufa = (ufa_t){
		.arena = config.arena,
		.filters = config.filters,
		.status = UFA_PENDING,
		.mode = "w",
	};

	if (config.filename != NULL) {
		ufa_open_file(ufa, config.filename);
	} else {
		SDL_ShowSaveFileDialog(
			ufa_file_callback, ufa,
			cf_app_get_window(),
			(const SDL_DialogFileFilter*)config.filters,
			config.num_filters,
			config.directory
		);
	}

	return (ufa_save_file_t*)ufa;
}

ufa_status_t
ufa_check_save_file(ufa_save_file_t* save_file) {
	return ((ufa_t*)save_file)->status;
}

const char*
ufa_get_save_file_error(ufa_save_file_t* save_file) {
	return SDL_GetError();
}

const char*
ufa_get_save_file_name(ufa_save_file_t* save_file) {
	return ((ufa_t*)save_file)->filename;
}

ufa_status_t
ufa_write_save_file(ufa_save_file_t* save_file, const void* buf, size_t* size) {
	ufa_t* ufa = (ufa_t*)save_file;
	if (ufa->stream == NULL) {
		*size = 0;
		return UFA_ERROR;
	}

	*size = SDL_WriteIO(ufa->stream, buf, *size);
	SDL_IOStatus status = SDL_GetIOStatus(ufa->stream);
	return status == SDL_IO_STATUS_READY || status == SDL_IO_STATUS_EOF ? UFA_OK : UFA_ERROR;
}

void
ufa_end_save_file(ufa_save_file_t* save_file) {
	ufa_t* ufa = (ufa_t*)save_file;
	if (ufa->stream) {
		SDL_CloseIO(ufa->stream);
		ufa->stream = NULL;
	}
}

// }}}

#else

// Common {{{

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <emscripten/em_macros.h>

EMSCRIPTEN_KEEPALIVE void*
ufa_web_malloc(void* alloc_ctx, size_t size) {
	return barena_malloc(alloc_ctx, size);
}

// }}}

// Open {{{

struct ufa_open_file_s {
	const char* filename;
	uint8_t status;
	int handle;
};

extern int
ufa_web_begin_open_file(
	const char* filter,
	void* alloc_ctx,
	const char** filename_ptr,
	uint8_t* status_ptr
);

extern void
ufa_web_end_open_file(int handle);

extern int
ufa_web_read_open_file(int handle, void* buf, size_t* size);

ufa_open_file_t*
ufa_begin_open_file(ufa_config_t config) {
	ufa_open_file_t* ufa = barena_memalign(config.arena, sizeof(ufa_open_file_t), _Alignof(ufa_open_file_t));
	*ufa = (ufa_open_file_t){
		.status = UFA_PENDING,
	};

	int len = 0;
	bool is_first = true;
	for (int i = 0; i < config.num_filters; ++i) {
		if (strcmp(config.filters[i].pattern, "*") == 0) { continue; }

		len += snprintf(NULL, 0, "%s.%s", is_first ? "" : ",", config.filters[i].pattern);
		is_first = false;
	}

	size_t buf_size = len + 1;
	char* filter = barena_memalign(config.arena, buf_size, _Alignof(char));
	char* buf_itr = filter;
	is_first = true;
	for (int i = 0; i < config.num_filters; ++i) {
		if (strcmp(config.filters[i].pattern, "*") == 0) { continue; }

		int part_len = snprintf(buf_itr, buf_size, "%s.%s", is_first ? "" : ",", config.filters[i].pattern);
		buf_size -= part_len;
		buf_itr += part_len;
		is_first = false;
	}

	ufa->handle = ufa_web_begin_open_file(filter, config.arena, &ufa->filename, &ufa->status);

	return ufa;
}

ufa_status_t
ufa_check_open_file(ufa_open_file_t* open_file) {
	return open_file->status;
}

const char*
ufa_get_open_file_error(ufa_open_file_t* open_file) {
	return NULL;
}

const char*
ufa_get_open_file_name(ufa_open_file_t* open_file) {
	return open_file->filename;
}

ufa_status_t
ufa_read_open_file(ufa_open_file_t* open_file, void* buf, size_t* size) {
	return ufa_web_read_open_file(open_file->handle, buf, size);
}

void
ufa_end_open_file(ufa_open_file_t* open_file) {
	ufa_web_end_open_file(open_file->handle);
}

// }}}

// Save {{{

struct ufa_save_file_s {
	const char* filename;
	uint8_t status;
	int handle;
};

extern int
ufa_web_begin_save_file(const char* filename);

extern int
ufa_web_write_save_file(int handle, const void* buf, size_t* size);

extern void
ufa_web_end_save_file(int handle);

ufa_save_file_t*
ufa_begin_save_file(ufa_config_t config) {
	ufa_save_file_t* ufa = barena_memalign(config.arena, sizeof(ufa_save_file_t), _Alignof(ufa_save_file_t));
	*ufa = (ufa_save_file_t){
		.status = UFA_OK,
		.filename = config.filename,
	};

	if (ufa->filename == NULL) {
		int size = snprintf(NULL, 0, "grid.%s", config.filters[0].pattern);
		char* filename = barena_memalign(config.arena, size + 1, _Alignof(char));
		snprintf(filename, size + 1, "grid.%s", config.filters[0].pattern);

		ufa->filename = filename;
	}

	ufa->handle = ufa_web_begin_save_file(ufa->filename);

	return ufa;
}

ufa_status_t
ufa_check_save_file(ufa_save_file_t* save_file) {
	return save_file->status;
}

const char*
ufa_get_save_file_error(ufa_save_file_t* save_file) {
	return NULL;
}

const char*
ufa_get_save_file_name(ufa_save_file_t* save_file) {
	return save_file->filename;
}

ufa_status_t
ufa_write_save_file(ufa_save_file_t* save_file, const void* buf, size_t* size) {
	return ufa_web_write_save_file(save_file->handle, buf, size);
}

void
ufa_end_save_file(ufa_save_file_t* save_file) {
	ufa_web_end_save_file(save_file->handle);
}

// }}}

#endif
