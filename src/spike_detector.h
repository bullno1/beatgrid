#ifndef SPIKE_DETECTOR_H
#define SPIKE_DETECTOR_H

#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    uint64_t window_start_timestamp;
    uint64_t frame_start_timestamp;
    uint64_t min_frame_time;
    uint64_t max_frame_time;
    uint64_t window_length;

    atomic_bool warning_flag;
} spike_detector_t;

void
spike_detector_init(spike_detector_t* detector, int sample_frames, int sample_rate);

void
spike_detector_begin(spike_detector_t* detector);

void
spike_detector_end(spike_detector_t* detector);

bool
spike_detector_check(spike_detector_t* detector);

#endif
