#ifndef UFA_H
#define UFA_H

#include <stddef.h>
#include <barena.h>

// Universal File Action

typedef enum {
	UFA_OK,
	UFA_PENDING,
	UFA_CANCELLED,
	UFA_ERROR,
} ufa_status_t;

typedef struct ufa_open_file_s ufa_open_file_t;
typedef struct ufa_save_file_s ufa_save_file_t;

typedef struct {
	const char* name;
	const char* pattern;
} ufa_filter_t;

ufa_open_file_t*
ufa_begin_open_file(
	barena_t* arena,
	const ufa_filter_t* filters,
	int num_filters
);

ufa_status_t
ufa_check_open_file(ufa_open_file_t* open_file);

const char*
ufa_get_open_file_error(ufa_open_file_t* open_file);

const char*
ufa_get_open_file_name(ufa_open_file_t* open_file);

ufa_status_t
ufa_read_open_file(ufa_open_file_t* open_file, void* buf, size_t* size);

void
ufa_end_open_file(ufa_open_file_t* open_file);

ufa_save_file_t*
ufa_begin_save_file(
	barena_t* arena,
	const ufa_filter_t* filters,
	int num_filters
);

ufa_status_t
ufa_check_save_file(ufa_save_file_t* save_file);

const char*
ufa_get_save_file_error(ufa_save_file_t* save_file);

const char*
ufa_get_save_file_name(ufa_save_file_t* save_file);

ufa_status_t
ufa_write_save_file(ufa_save_file_t* save_file, const void* buf, size_t* size);

void
ufa_end_save_file(ufa_save_file_t* save_file);

#endif
