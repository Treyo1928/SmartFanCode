#include "ToneSongs.h"

using namespace std;

#define TONE_PIN 12 // Pin you have speaker/piezo connected to (be sure to include a 100 ohm resistor).

// Simple alarm tone (rising and falling)
const int alarmMelody[] = {
    440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, // Rising A4 to A5
    880, 831, 784, 740, 698, 659, 622, 587, 554, 523, 494, 466, 440, // Falling A5 to A4
    440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, // Rising again
    880, 831, 784, 740, 698, 659, 622, 587, 554, 523, 494, 466, 440  // Falling again
};

const unsigned int alarmDuration[] = {
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, // Rising
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, // Falling
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, // Rising
    80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80  // Falling
};

const int sizeofAlarm = sizeof(alarmMelody) / sizeof(alarmMelody[0]);

// Emergency siren (alternating tones)
const int sirenMelody[] = {
    880, 0, 880, 0, 880, 0, 880, 0, // High tone (A5) with pauses
    660, 0, 660, 0, 660, 0, 660, 0, // Low tone (E5) with pauses
    880, 0, 880, 0, 880, 0, 880, 0, // Repeat high
    660, 0, 660, 0, 660, 0, 660, 0  // Repeat low
};

const unsigned int sirenDuration[] = {
    200, 50, 200, 50, 200, 50, 200, 50, // High tone pattern
    200, 50, 200, 50, 200, 50, 200, 50, // Low tone pattern
    200, 50, 200, 50, 200, 50, 200, 50, // Repeat high
    200, 50, 200, 50, 200, 50, 200, 50  // Repeat low
};

const int sizeofSiren = sizeof(sirenMelody) / sizeof(sirenMelody[0]);

const unsigned int parisMelody[] = {
    349, 0, 262, 0, 0, 0, 415, 0, 262, 0, 0, 0, 
    392, 0, 262, 0, 0, 0, 349, 0, 262, 0, 524, 0, 0, 0 };

const unsigned int parisDuration[] = {
    187, 50, 187, 50, 375, 50, 187, 50, 187, 50, 375, 50, 
    187, 50, 187, 50, 375, 50, 187, 50, 187, 50, 187, 50,
     187, 50 
};

const int *const melodies[] = {
    alarmMelody,
    sirenMelody};

const unsigned int *const durations[] = {
    alarmDuration,
    sirenDuration};

const int songSizes[] = {
    sizeofAlarm,
    sizeofSiren};

const char *const songNames[] = {
    "Alarm",
    "Emergency Siren"};

unsigned long lastNoteTime = 0;
const int numSongs = sizeof(songNames) / sizeof(songNames[0]);

void songLoop(int song)
{
    // Check if we need to start the song over
    if (NoteIndex >= songSizes[song] || NoteIndex == -1)
    {
        Serial.print("Now Playing: ");
        Serial.println(songNames[song]);
        NoteIndex = 0;
        lastNoteTime = millis();
        TimerFreeTone(TONE_PIN, melodies[song][NoteIndex], durations[song][NoteIndex], 10);
        return;
    }

    // Check if it's time to play the next note
    if ((millis() - lastNoteTime) >= durations[song][NoteIndex])
    {
        lastNoteTime = millis();
        NoteIndex++;
        
        // Check if we've reached the end of the song
        if (NoteIndex >= songSizes[song])
        {
            Serial.println("Song is over");
            NoteIndex = -1;  // Set to -1 to indicate song should restart next time
            return;
        }
        
        // Play the next note
        TimerFreeTone(TONE_PIN, melodies[song][NoteIndex], durations[song][NoteIndex], 10);
    }
}

void resetSongs()
{
    NoteIndex = -1;
}