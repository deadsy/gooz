/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_INC_CONST_H
#define GGM_SRC_INC_CONST_H

#ifndef GGM_SRC_INC_GGM_H
#warning "please include this file using ggm.h"
#endif

/******************************************************************************
 * Root Module Port Number Limits.
 */

#define MAX_AUDIO_IN 2		/* max number of audio input ports */
#define MAX_AUDIO_OUT 2		/* max number of audio output ports */
#define MAX_MIDI_IN 1		/* max number of MIDI input ports */
#define MAX_MIDI_OUT 1		/* max number of MIDI output ports */

#define MAX_AUDIO_PORTS (MAX_AUDIO_IN + MAX_AUDIO_OUT)

/******************************************************************************
 * Audio Constants.
 */

/* AudioSampleFrequency is the sample frequency for audio (Hz) */
#define AudioSampleFrequency (48000U)

/* AudioBufferSize is the number of float samples per audio buffer */
#define AudioBufferSize (128)

/******************************************************************************
 * Derived/Fundanmental Constants (don't modify).
 */

/* Pi (3.14159...) */
#define Pi (3.1415926535897932384626433832795f)

/* Tau (2 * Pi) */
#define Tau (2.f * Pi)

/* AudioSamplePeriod is the sample period for audio (seconds) */
#define AudioSamplePeriod (1.f / (float)AudioSampleFrequency)

/* SecsPerAudioBuffer is the audio duration for a single audio buffer */
#define SecsPerAudioBuffer ((float)AudioBufferSize / (float)AudioSampleFrequency)

/* FullCycle is a full uint32_t phase count */
#define FullCycle (1ULL << 32)

/* HalfCycle is a half uint32_t phase count */
#define HalfCycle (1U << 31)

/* QuarterCycle is a quarter uint32_t phase count */
#define QuarterCycle (1U << 30)

/* FrequencyScale scales a frequency value to a uint32_t phase step value */
#define FrequencyScale ((float)FullCycle / (float)AudioSampleFrequency)

/* PhaseScale scales a phase value to a uint32_t phase step value */
#define PhaseScale ((float)FullCycle / Tau)

/* SecsPerMin (seconds per minute) */
#define SecsPerMin (60.0f)

/*****************************************************************************/

#endif				/* GGM_SRC_INC_CONST_H */

/*****************************************************************************/
