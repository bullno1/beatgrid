#include "audio_callback.h"
#include "tribuf.h"
#include <math.h>
#include <bgame/allocator.h>
#include <cute_math.h>

#define RAMP_TIME_S 0.01f

struct audio_callback_state_s {
	audio_callback_config_t config;

	tribuf_t cmd_queue;
	audio_cmd_t cmd_buf[3];

	bg_pipeline_t* pipeline;
	bg_output_t output;
	bool playing;
	float volume_gain;

	float last_sample;
	float output_buf[];
};

void SDLCALL
audio_callback(
	void* userdata,
	SDL_AudioStream* stream,
    int additional_amount,
	int total_amount
) {
	audio_callback_state_t* state = userdata;

	// Handle incoming command
	audio_cmd_t* cmd = tribuf_begin_recv(&state->cmd_queue);
	if (cmd != NULL) {
		state->playing = cmd->playing;
		bg_pipeline_set_params(state->pipeline, (bg_pipeline_params_t){
			.dt = 1.f / (float)state->config.sample_rate,
			.num_outputs = 1,
			.outputs = &state->output,
			.grid_params = cmd->grid_params,
		});
		bg_pipeline_build(state->pipeline, state->config.node_registry, &cmd->grid);
		tribuf_end_recv(&state->cmd_queue);
	}

	// Generate audio
	float gain_rate = (1.f / ((float)state->config.sample_rate * RAMP_TIME_S)) * (state->playing ? 1.f : -1.f);

	if (state->playing || state->volume_gain > 0.f) {
		int num_frames_needed = additional_amount / sizeof(float);
		for (int i = 0; i < num_frames_needed; ++i) {
			bg_pipeline_process(state->pipeline);

			float sample = (state->output.value / (float)state->output.count) * state->volume_gain;
			if (!isnan(sample)) {  // NaN is emitted as reset signal
				state->output_buf[i] = state->last_sample = sample;
			} else {
				state->output_buf[i] = state->last_sample;
			}

			state->volume_gain = cf_clamp(state->volume_gain + gain_rate, 0.f, 1.f);
		}
	} else {
		memset(state->output_buf, 0, additional_amount);
	}

	SDL_PutAudioStreamData(stream, state->output_buf, additional_amount);
}

void
audio_callback_reinit(audio_callback_state_t** state_ptr, audio_callback_config_t config) {
	audio_callback_state_t* state = bgame_realloc(
		*state_ptr,
		sizeof(audio_callback_state_t) + sizeof(float) * config.frames_per_call,
		config.allocator
	);
	if (*state_ptr == NULL) {
		*state = (audio_callback_state_t){
			.config = config,
		};
	} else {
		state->config = config;
	}

	bg_pipeline_reinit(&state->pipeline, config.allocator);
	bg_grid_reinit(&state->cmd_buf[0].grid, config.allocator);
	bg_grid_reinit(&state->cmd_buf[1].grid, config.allocator);
	bg_grid_reinit(&state->cmd_buf[2].grid, config.allocator);
	tribuf_init(&state->cmd_queue, &state->cmd_buf[0], sizeof(state->cmd_buf[0]));

	*state_ptr = state;
}

void
audio_callback_cleanup(audio_callback_state_t** state_ptr) {
	audio_callback_state_t* state = *state_ptr;
	if (state == NULL) { return; }

	bg_grid_cleanup(&state->cmd_buf[0].grid);
	bg_grid_cleanup(&state->cmd_buf[1].grid);
	bg_grid_cleanup(&state->cmd_buf[2].grid);
	bg_pipeline_cleanup(&state->pipeline);
	bgame_free(state, state->config.allocator);

	*state_ptr = NULL;
}

audio_cmd_t*
audio_callback_control_begin(audio_callback_state_t* state) {
	return tribuf_begin_send(&state->cmd_queue);
}

void
audio_callback_control_end(audio_callback_state_t* state) {
	tribuf_end_send(&state->cmd_queue);
}

void
audio_callback_sync(audio_callback_state_t* state) {
	tribuf_try_swap(&state->cmd_queue);
}
