#include "ufa.h"

#ifndef __EMSCRIPTEN__

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_iostream.h>
#include <cute_app.h>

_Static_assert(sizeof(SDL_DialogFileFilter) == sizeof(ufa_filter_t), "SDL_DialogFileFilter has a different size");
_Static_assert(offsetof(SDL_DialogFileFilter, name) == offsetof(ufa_filter_t, name), "SDL_DialogFileFilter.name has a different offset");
_Static_assert(offsetof(SDL_DialogFileFilter, pattern) == offsetof(ufa_filter_t, pattern), "SDL_DialogFileFilter.pattern has a different offset");

typedef struct {
	barena_t* arena;
	const char* filename;
	ufa_status_t status;
	SDL_IOStream* stream;
	const char* mode;
} ufa_t;

static void SDLCALL
ufa_file_callback(void* userdata, const char* const* filelist, int filter) {
	ufa_t* ufa = userdata;

	if (filelist == NULL) {
		ufa->status = UFA_ERROR;
	} else if (*filelist == NULL) {
		ufa->status = UFA_CANCELLED;
	} else {
		const char* filename = *filelist;
		size_t len = strlen(filename);
		char* filename_copy = barena_memalign(ufa->arena, len + 1, _Alignof(char));
		memcpy(filename_copy, filename, len + 1);
		ufa->filename = filename_copy;

		ufa->stream = SDL_IOFromFile(filename, ufa->mode);
		ufa->status = ufa->stream != NULL ? UFA_OK : UFA_ERROR;
	}
}

ufa_open_file_t*
ufa_begin_open_file(
	barena_t* arena,
	const ufa_filter_t* filters,
	int num_filters
) {
	ufa_t* ufa = barena_memalign(arena, sizeof(ufa_t), _Alignof(ufa_t));
	*ufa = (ufa_t){
		.arena = arena,
		.status = UFA_PENDING,
		.mode = "r",
	};

	SDL_ShowOpenFileDialog(
		ufa_file_callback, ufa,
		cf_app_get_window(),
		(const SDL_DialogFileFilter*)filters,
		num_filters,
		NULL,
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

ufa_save_file_t*
ufa_begin_save_file(
	barena_t* arena,
	const ufa_filter_t* filters,
	int num_filters
) {
	ufa_t* ufa = barena_memalign(arena, sizeof(ufa_t), _Alignof(ufa_t));
	*ufa = (ufa_t){
		.arena = arena,
		.status = UFA_PENDING,
		.mode = "w",
	};

	SDL_ShowSaveFileDialog(
		ufa_file_callback, ufa,
		cf_app_get_window(),
		(const SDL_DialogFileFilter*)filters,
		num_filters,
		NULL
	);

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

#else

#endif
