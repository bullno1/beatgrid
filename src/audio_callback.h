#ifndef BEATGRID_AUDIO_CALLBACK_H
#define BEATGRID_AUDIO_CALLBACK_H

#include <SDL3/SDL_audio.h>
#include "grid.h"

struct bgame_allocator_s;

typedef struct audio_callback_state_s audio_callback_state_t;

typedef struct {
	bool playing;
	bg_grid_params_t grid_params;
	bg_grid_t grid;
} audio_cmd_t;

typedef struct {
	struct bgame_allocator_s* allocator;
	bg_node_registry_t* node_registry;
	bg_grid_params_t grid_param;
	int sample_rate;
	int frames_per_call;
} audio_callback_config_t;

void SDLCALL
audio_callback(
	void* userdata,
	SDL_AudioStream* stream,
    int additional_amount,
	int total_amount
);

void
audio_callback_reinit(audio_callback_state_t** state_ptr, audio_callback_config_t config);

void
audio_callback_cleanup(audio_callback_state_t** state_ptr);

audio_cmd_t*
audio_callback_control_begin(audio_callback_state_t* state);

void
audio_callback_control_end(audio_callback_state_t* state);

void
audio_callback_sync(audio_callback_state_t* state);

#endif
