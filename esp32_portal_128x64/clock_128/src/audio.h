#pragma once

// audio out of the speaker on the A0 JST connector (GPIO12).
// the ESP32-S3 has no DAC, the samples go out as PDM (a 1-bit stream) from I2S0, the speaker amplifier's
// input filtering turns that into the analog signal.
// the I2S driver and the audio task are started the first time sound is needed

// at boot: holds the speaker pin low, quiet until there's sound
void audio_begin();

// starts or stops a continuous sine beep
void audio_beep(bool on);

// of the beep, can be changed while it plays. default 440 Hz
void audio_set_freq(int hz);

// percent of full scale. default 30
void audio_set_volume(int percent);

bool audio_beeping();
