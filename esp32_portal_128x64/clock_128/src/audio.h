#pragma once
#include <Arduino.h>
#include <vector>

// audio out of a MAX98357A I2S amplifier: BCLK on A4, LRC on A3, DIN on A2.
// plays a sine beep or an mp3 of the file system's /sounds folder (made by sounds/convert_sounds.py), one at a
// time: starting one stops the other, with a short fade.
// the I2S driver and the audio task are started the first time sound is needed, while nothing plays the I2S
// clocks are stopped and the amplifier is off. at volume 0 a beep or a sound doesn't start at all

// at boot, after the file system is mounted: lists the sounds
void audio_begin();

// the mp3 files in /sounds, names without the folder, sorted
const std::vector<String>& audio_sounds();

// starts or stops a continuous sine beep
void audio_beep(bool on);

// plays /sounds/<name> once
void audio_play(const char* name);

// stops whatever plays
void audio_stop();

// of the beep, can be changed while it plays. default 440 Hz
void audio_set_freq(int hz);

// percent, the amplitude is its square (more even steps of loudness). default 30
void audio_set_volume(int percent);

bool audio_beeping();
bool audio_playing_file();
