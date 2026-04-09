#include "spike_detector.h"
#include <SDL3/SDL_timer.h>

void
spike_detector_init(spike_detector_t* detector, int sample_frames, int sample_rate) {
	*detector = (spike_detector_t){
		.min_frame_time = UINT64_MAX,
   	};

    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    detector->window_length = perf_freq * 2;
	detector->warning_flag = false;
}

void
spike_detector_begin(spike_detector_t* detector) {
    uint64_t now = SDL_GetPerformanceCounter();
	detector->frame_start_timestamp = now;
	if (detector->window_start_timestamp == 0) {
		detector->window_start_timestamp = now;
	}
}

void
spike_detector_end(spike_detector_t* detector) {
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t frame_time = now - detector->frame_start_timestamp;

    if (frame_time > detector->max_frame_time) {
        detector->max_frame_time = frame_time;
	}

    if (frame_time < detector->min_frame_time) {
        detector->min_frame_time = frame_time;
	}

    uint64_t window_time = now - detector->window_start_timestamp;
	if (window_time >= detector->window_length) {
		bool warning = false;
		if (detector->max_frame_time > 0 && detector->min_frame_time < UINT64_MAX) {
			double ratio = (double)detector->max_frame_time / (double)detector->min_frame_time;
			if (ratio >= 3.0) {
				warning = true;
			}
		}

		atomic_store(&detector->warning_flag, warning);

		detector->window_start_timestamp = now;
		detector->max_frame_time = 0;
		detector->min_frame_time = UINT64_MAX;
	}
}

bool
spike_detector_check(spike_detector_t* detector) {
	return atomic_load(&detector->warning_flag);
}
