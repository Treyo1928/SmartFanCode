#include <TimerFreeTone.h>

using namespace std;

#define TONE_PIN 12 // Pin you have speaker/piezo connected to (be sure to include a 100 ohm resistor).

int parisMelody[] = {349, 0, 262, 0, 0, 0, 415, 0, 262, 0, 0, 0, 392, 0, 262, 0, 0, 0, 349, 0, 262, 0, 524, 0, 0, 0 };
int parisDuration[] = {187, 50, 187, 50, 375, 50, 187, 50, 187, 50, 375, 50, 187, 50, 187, 50, 375, 50, 187, 50, 187, 50, 187, 50, 187, 50 };

int travisMel[] = {740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 
                   740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 
                   740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 1108, 0, 0, 0, 740, 0, 988, 0, 1108, 0, 1176, 0, 1176, 0, 
                   1108, 0, 1108, 0, 988, 0, 988, 0, 880, 0, 880, 0, 784, 0, 740, 0, 0, 0, 740, 0, 740, 0, 988, 0, 988, 0, 740, 0, 740, 0, 660, 0, 0, 0, 
                   740, 0, 740, 0, 988, 0, 988, 0, 740, 0, 740, 0, 660, 0, 0, 0, 740, 0, 740, 0, 988, 0, 988, 0, 740, 0, 740, 0, 660, 0, 0, 0, 660, 0, 
                   588, 0, 660, 0, 588, 0, 660, 0, 588, 0, 740, 0, 988, 0, 740, 0, 988, 0, 740, 0, 988, 0, 740, 0, 988, 0};

int travisDur[] =  {157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 
                    157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 
                    157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 
                    157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 
                    157, 50, 157, 50, 157, 50, 1263, 50, 315, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 1350, 50, 350, 50, 157, 50, 157, 50, 
                    157, 50, 157, 50, 157, 50, 157, 50, 1350, 50, 350, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 157, 50, 350, 50, 350, 50, 350, 50, 
                    350, 50, 350, 50, 350, 50, 350, 50, 350, 50};

int jcoleMel[] = {1048, 0, 1048, 0, 932, 0, 830, 0, 932, 0, 830, 0, 784, 0, 
                  698, 0, 524, 0, 524, 0, 0, 0, 
                  1048, 0, 932, 0, 830, 0, 932, 0, 830, 0, 784, 0, 
                  698, 0, 524, 0, 524, 0};

int jcoleDur[] = {280, 50, 125, 50, 125, 50, 125, 50, 125, 50, 125, 50, 125, 50, 
                  280, 50, 280, 50, 280, 50, 
                  550, 50, 125, 50, 125, 50, 125, 50, 125, 50, 125, 50, 125, 50, 
                  280, 50, 280, 50, 280, 50};

int diasMel[] = {622, 0, 588, 0, 622, 0, 524, 0, 588, 0, 466, 0, 524, 0, 440, 0 };
int diasDur[] = {315, 50, 315, 50, 315, 50, 315, 50, 315, 50, 315, 50, 315, 50, 315, 50 };

int highMel[] = {1244, 0, 1320, 0, 1480, 1244, 0, 0, 0, 1108, 0, 988, 0, 1108, 932, 0, 0, 0 };
int highDur [] = {473, 50, 315, 50, 315, 500, 50, 947, 50, 473, 50, 315, 50, 315, 500, 50, 947, 50 };

int sizeofSong1 = sizeof(parisMelody) / sizeof(parisMelody[0]);
int sizeofSong2 = sizeof(travisMel) / sizeof(travisMel[0]);
int sizeofSong3 = sizeof(jcoleMel) / sizeof(jcoleMel[0]);
int sizeofSong4 = sizeof(diasMel) / sizeof(diasMel[0]);
int sizeofSong5 = sizeof(highMel) / sizeof(highMel[0]);

int songNum;

void playSong(int song);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for Serial port to connect. Needed for native USB boards
  }
  Serial.println("Setup Started");
}
void loop() {
  while (Serial.available() == 0) {
    // Do nothing, just wait
  }
  String input = Serial.readStringUntil('\n');
  int songNum = input.toInt();

  playSong(songNum);
}

void playSong(int song) {
  if (song == 1){
    for (int thisNote = 0; thisNote < sizeofSong1; thisNote++) { // Loop through the notes in the array.
      TimerFreeTone(TONE_PIN, parisMelody[thisNote], parisDuration[thisNote]); // Play thisNote for duration.
      //delay(50); // Short delay between notes.
    } 
  }
  else if (song == 2){
    for (int thisNote = 0; thisNote < sizeofSong2; thisNote++) { // Loop through the notes in the array.
      TimerFreeTone(TONE_PIN, travisMel[thisNote], travisDur[thisNote]);
    }
  }
  else if (song == 3){
    for (int thisNote = 0; thisNote < sizeofSong3; thisNote++) { // Loop through the notes in the array.
      TimerFreeTone(TONE_PIN, jcoleMel[thisNote], jcoleDur[thisNote]);
    }
  }
  else if (song == 4) {
    for (int thisNote = 0; thisNote < sizeofSong4; thisNote++) { // Loop through the notes in the array.
      TimerFreeTone(TONE_PIN, diasMel[thisNote], diasDur[thisNote]);
    }
  }
  else if (song == 5) {
    for (int thisNote = 0; thisNote < sizeofSong5; thisNote++) { // Loop through the notes in the array.
      TimerFreeTone(TONE_PIN, highMel[thisNote], highDur[thisNote]);
    }
  }
}
