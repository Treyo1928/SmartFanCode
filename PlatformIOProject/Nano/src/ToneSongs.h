#ifndef ToneSongs_h
#define ToneSongs_h
#include "TimerFreeTone.h"
#include <string.h>

void songLoop(int song);
void resetSongs();
#pragma once
extern int NoteIndex;
extern const char* const songNames[];
extern const int numSongs;
#endif