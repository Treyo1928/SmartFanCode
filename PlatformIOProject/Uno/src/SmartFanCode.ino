#include <Wire.h> // Needed for I2C
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // This and GFX drive the display
#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#include <RTClib.h>
#include <TimeLib.h> // Allows us to set DateTime and keep track of the time after syncing to the RTC
#include <TimeAlarms.h>
#include <EEPROM.h>
#include "custom_chars.h"  // The header file we made to store the data for what our custom characters look like
#include "TimerFreeTone.h" // A custom tone libary that frees up TIMER2 so the IR Reviever can still function
#include "ToneSongs.h"     // Custom script for playing songs

#define NEO_PIN 2
#define NUM_LEDS 16
#define IR_PIN 4
#define temperaturePin A0

const int MOTOR_PIN = 9;
const int BUZZER_PIN = 5;
const int SCREEN_WIDTH = 128; // OLED display width, in pixels
const int SCREEN_HEIGHT = 64; // OLED display height, in pixels

// const int alarmSetBeforeEEPROM = 0;
// const int alarmHourEEPROM = 1;
// const int alarmMinuteEEPROM = 2;

// typedef struct flashStruct
// { // Nano 33 BLE Sense can't use EEPROM but instead this nonsense
//   int alarmSetBeforeEEPROM = 0;
//   int alarmHourEEPROM = 1;
//   int alarmMinuteEEPROM = 2;
//   int fanSpeed = 0;
//   int alarmSong = 0;
//   bool alarmEnabled = true;
// } flashPrefs;
// NanoBLEFlashPrefs myFlashPrefs;
// flashPrefs prefs;

typedef struct eepromStruct
{
  int alarmSetBeforeEEPROM;
  int alarmHourEEPROM;
  int alarmMinuteEEPROM;
  int fanSpeed;
  int alarmSong;
  int neoPixelMode;
  int neoColor;
  int neoBrightness;
  int neoSpeed;
  bool alarmEnabled;
  int scrollOffset;
} eepromPrefs;

eepromPrefs prefs;

int menu = 0;
int pressedButton = 0;
int menuSelectionIndex = 0;
int subMenuSelectionIndex = 0;
int fanSpeed = 0;
bool poweredOn = true;
bool alarmGoingOff = false;
bool alarmEnabled = false;
volatile bool alarmJustTriggered = false; // Needs to be volatile because the bool is being set inside an interrupt
unsigned long alarmWentOffTime;
unsigned long lastNeoUpdate = 0;
unsigned long lastTempUpdate = 0;
unsigned long lastIRInput = 0;
int neoPixelMode = 0;
const int neoModes = 4;
int neoPixelIndex = 1;
int neoPixelFadeDirection = -1;
int neoColor = 0;            // 0 = all white, 1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = purple, 6 = pink, 7 = rainbow, 8 = off
const int neoColorCount = 9; // Add 1 because of modulus
int maxNeoBrightness = 25;
int neoBrightness = maxNeoBrightness;
const int slowestNeoSpeed = 150;
int neoSpeed = 70; // This is a percentage
int neoWaitTime = 0;
float temperatureF = 65;

const int MAX_VISIBLE_ITEMS = 4; // Number of items that can fit on screen
int scrollOffset = 0;            // Tracks which item is at the top of visible area
const int TOTAL_SONGS = 7;       // Total number of songs

int alarmHour = 12;
int alarmMinute = 30;
int tempTimeHour = 0;
int tempTimeMinute = 0;
int tempTimeSecond = 0;

int NoteIndex = 0;
int alarmSong = 0;

/*
  IR REMOTE BUTTON TO CODE CONVERSION
  -----------------------------------
  162 - Power              226 - Menu
  34 - Test    2 - Plus    194 - Back
  224 - Prev   168 - Play  144 - Next
  104 - 0      152 - Minus 176 - C
  48 - 1       24 - 2      122 - 3
  16 - 4       56 - 5      90 - 6
  66 - 7       74 - 8      82 - 9
*/
/*
  IR REMOTE BUTTON TO CODE CONVERSION
  -----------------------------------
  0 - Vol-     1 - Play    2 - Vol+
  4 - Setup    5 - Up      6 - Stop
  8 - Left     9 - Enter   10 - Right
  12 -         13 - Down   14 - Back
  16 - 1       17 - 2      18 - 3
  20 - 4       21 - 5      22 - 6
  24 - 7       25 - 8      26 - 9
*/

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_NeoPixel ring(NUM_LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);
IRrecv receiver(IR_PIN);
RTC_DS1307 rtc;
AlarmID_t mainAlarm;

void checkForIR();
void mainMenu();
void setAlarmMenu();
void setAlarmSound();
void setTimeMenu();
void neoPixelConfigMenu();
void displayCenteredText(String text, int height);
void displayRightAlignedText(String text, int height);
void drawDisplayHeader();
void menuSelectionMenu();
void alarmGoingOffMenu();
void restartProgram();
void animateNeoPixel();
void triggerAlarm();
void updateFanSpeed();
void updateTemperature(); // Define the functions before the loop so I can have their contents after the loop
void loadPreferences();
void savePreferences();
void resetEEPROM();
void neoModeUpdated();
int *calculateColors(int color);

void setup()
{
  Serial.begin(9600);
  Serial.println("Setup Started");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    // while (true);
  }

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    // abort();
  }
  display.setTextColor(WHITE);

  ring.begin();
  ring.setBrightness(neoBrightness);
  ring.show(); // Dims the LEDs as to not blind you when you first turn it on
  receiver.enableIRIn();

  DateTime now = rtc.now();
  setTime(now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  EEPROM.begin();
  loadPreferences();
  if (prefs.alarmSetBeforeEEPROM != 91)
  {
    prefs.alarmHourEEPROM = 12;
    prefs.alarmMinuteEEPROM = 30;
    prefs.alarmSetBeforeEEPROM = 91;
    prefs.fanSpeed = 0;
    prefs.alarmSong = 0;
    prefs.neoPixelMode = 0;
    prefs.neoColor = 0;
    prefs.neoBrightness = 25;
    prefs.alarmEnabled = true;
    prefs.neoSpeed = 70;
    prefs.scrollOffset = 0;
    savePreferences();
  }
  else
  {
    Serial.println("Setting variables to prefs");
    alarmHour = prefs.alarmHourEEPROM;
    alarmMinute = prefs.alarmMinuteEEPROM;
    alarmEnabled = prefs.alarmEnabled;
    fanSpeed = prefs.fanSpeed;
    alarmSong = prefs.alarmSong;
    neoColor = prefs.neoColor;
    neoPixelMode = prefs.neoPixelMode;
    neoBrightness = prefs.neoBrightness;
    neoSpeed = prefs.neoSpeed;
    scrollOffset = prefs.scrollOffset;
  }
  mainAlarm = Alarm.alarmRepeat(alarmHour, alarmMinute, 0, triggerAlarm); // Alarm set for alarmHour:alarmMinute:00 | Calls the triggerAlarm method

  pinMode(MOTOR_PIN, OUTPUT);
  updateFanSpeed();
  neoModeUpdated();

  NoteIndex = 0;

  Serial.println("Setup Complete");
}

void loop()
{
  Alarm.delay(0); // Needed to refresh the time in the TimeAlarms library
  checkForIR();

  animateNeoPixel();
  if (millis() - lastTempUpdate >= 1000) // If a second has passed, update the temp
  {
    updateTemperature();
    lastTempUpdate = millis();
  }

  if (alarmJustTriggered)
  {
    if (!alarmEnabled)
    {
      alarmJustTriggered = false;
      return;
    }
    Serial.println("AlarmJustTriggeredSetups");
    menu = 6;
    poweredOn = true;
    alarmGoingOff = true;
    alarmWentOffTime = millis();
    alarmJustTriggered = false;
    Alarm.disable(mainAlarm);
  }

  if (!poweredOn && !alarmGoingOff)
  { // Stops the code if the device is off and the alarm isn't going off
    if (pressedButton == 6)
      poweredOn = true; // Turns device on if power button is pressed
    return;
  }

  switch (menu)
  {
  case 0:
    mainMenu();
    break;
  case 1:
    setAlarmMenu();
    break;
  case 2:
    setAlarmSound();
    break;
  case 3:
    setTimeMenu();
    break;
  case 4: // Reset EEPROM Button
    resetEEPROM();
    break;
  case 5:
    menuSelectionMenu();
    break;
  case 6:
    alarmGoingOffMenu();
    break;
  }
}

void mainMenu()
{
  switch (pressedButton)
  {
  case 4: // Setup
    menu = 5;
    menuSelectionIndex = 0;
    break;
  case 6: // Stop / Mode
    poweredOn = false;
    display.clearDisplay();
    display.display();
    pressedButton = -2;
    break;

  case 5: // Up Arrow
    fanSpeed += 5;
    if (fanSpeed > 100)
      fanSpeed = 100;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 13:
    fanSpeed += -5;
    if (fanSpeed < 0)
      fanSpeed = 0;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 12:
    if (fanSpeed == 0)
      fanSpeed = 100;
    else
      fanSpeed = 0;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 16:
    fanSpeed = 10;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 17:
    fanSpeed = 20;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 18:
    fanSpeed = 30;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 20:
    fanSpeed = 40;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 21:
    fanSpeed = 50;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 22:
    fanSpeed = 60;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 24:
    fanSpeed = 70;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 25:
    fanSpeed = 80;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;
  case 26:
    fanSpeed = 90;
    prefs.fanSpeed = fanSpeed;
    savePreferences();
    updateFanSpeed();
    break;

  // Neo Pixel Commands
  case 10:
    neoBrightness = maxNeoBrightness;
    switch (menuSelectionIndex)
    {
    case 0: // Changing animation
      neoPixelMode += 1;
      neoPixelMode %= neoModes;
      break;
    case 1: // Changing color
      neoColor += 1;
      neoColor %= neoColorCount;
      break;
    case 2: // Changing brightness
      maxNeoBrightness += 1;
      if (maxNeoBrightness > 40)
        maxNeoBrightness = 40;
      neoBrightness = maxNeoBrightness;
      break;
    case 3: // Changing speed
      neoSpeed += 5;
      if (neoSpeed > 100)
        neoSpeed = 100;
      break;
    }
    neoPixelIndex = 0;
    neoModeUpdated();
    break;
  case 8:
    neoBrightness = maxNeoBrightness;
    switch (menuSelectionIndex)
    {
    case 0: // Changing animation
      neoPixelMode += 3;
      neoPixelMode %= neoModes;
      break;
    case 1: // Changing color
      neoColor += neoColorCount - 1;
      neoColor %= neoColorCount;
      break;
    case 2: // Changing brightness
      maxNeoBrightness -= 1;
      if (maxNeoBrightness < 0)
        maxNeoBrightness = 0;
      neoBrightness = maxNeoBrightness;
    case 3: // Changing speed
      neoSpeed -= 5;
      if (neoSpeed < 0)
        neoSpeed = 0;
      break;
    }
    neoPixelIndex = 0;
    neoModeUpdated();
    break;
  // case 0: // This button is broken
  //   neoColor += neoColorCount - 1;
  //   neoColor %= neoColorCount;
  //   neoPixelIndex = 0;
  //   neoModeUpdated();
  //   break;
  case 2:
    // neoColor += 1;
    // neoColor %= neoColorCount;
    // neoPixelIndex = 0;
    // neoModeUpdated();

    // Used to be used for changing color but now changes neo editing mode
    menuSelectionIndex++;
    menuSelectionIndex %= 4;
    break;

    /*
    IR REMOTE BUTTON TO CODE CONVERSION
    -----------------------------------
    0 - Vol-     1 - Play    2 - Vol+
    4 - Setup    5 - Up      6 - Stop
    8 - Left     9 - Enter   10 - Right
    12 -         13 - Down   14 - Back
    16 - 1       17 - 2      18 - 3
    20 - 4       21 - 5      22 - 6
    24 - 7       25 - 8      26 - 9
    */
  default:
    display.clearDisplay();
    drawDisplayHeader();

    display.setTextSize(1);
    String temperatureText = String(int(temperatureF)) + "F"; // Prints degree symbol with the temperature
    displayRightAlignedText(temperatureText, 55);

    char alarmText[5];
    sprintf(alarmText, "%02d:%02d", alarmHour, alarmMinute);
    displayCenteredText("Alarm Trigger Time", 16);
    display.drawLine(10, 24, 117, 24, SSD1306_WHITE);
    displayCenteredText(String(alarmText), 28);

    String fanSpeedString = String(fanSpeed) + "% Fan Speed";
    displayCenteredText(fanSpeedString, 37);

    String neoEditModeString;
    switch (menuSelectionIndex)
    {
    case 0:
      neoEditModeString = "Mode: ";
      switch (neoPixelMode)
      {
      case 0:
        neoEditModeString += "Wipe";
        break;
      case 1:
        neoEditModeString += "Cycle";
        break;
      case 2:
        neoEditModeString += "Breathe";
        break;
      case 3:
        neoEditModeString += "Solid Color";
        break;
      }
      break;
    case 1:
      neoEditModeString = "Color: ";
      switch (neoColor)
      { // neoColor guide: 0 = all white, 1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = purple, 6 = pink, 7 = rainbow
      case 0:
        neoEditModeString += "White";
        break;
      case 1:
        neoEditModeString += "Red";
        break;
      case 2:
        neoEditModeString += "Green";
        break;
      case 3:
        neoEditModeString += "Blue";
        break;
      case 4:
        neoEditModeString += "Yellow";
        break;
      case 5:
        neoEditModeString += "Purple";
        break;
      case 6:
        neoEditModeString += "Pink";
        break;
      case 7:
        neoEditModeString += "Rainbow";
        break;
      case 8:
        neoEditModeString += "Off";
        break;
      }
      break;
    case 2:
      neoEditModeString = "Brightness: ";
      neoEditModeString += String(int((float(maxNeoBrightness) / 40.00) * 100)) + "%";
      break;
    case 3:
      neoEditModeString = "LED Speed: ";
      neoEditModeString += String(neoSpeed) + "%";
      break;
    }
    displayCenteredText(neoEditModeString, 46);

    display.display();
    break;
  }
}

void checkForIR()
{
  if (receiver.decode())
  {
    pressedButton = receiver.decodedIRData.command;
    receiver.resume();

    if (millis() - lastIRInput < 400)
      pressedButton = -2; // Software debouncing for remote input
    else
    {
      lastIRInput = millis();
      Serial.print("Button Preseed: ");
      Serial.println(pressedButton);
    }
  }
  else
    pressedButton = -2;
}

void drawDisplayHeader()
{
  if (pressedButton == 1)
  {
    Serial.println("Alarm Button Pressed");
    alarmEnabled = !alarmEnabled;
    prefs.alarmEnabled = alarmEnabled;
    savePreferences();
  }

  display.setTextSize(2);
  char timeText[9]; // Buffer for "HH:MM:SS\0"
  sprintf(timeText, "%02d:%02d:%02d", hour(), minute(), second());
  displayCenteredText(String(timeText), 0);
  display.drawBitmap(4, 4, alarmEnabled ? alarmOnIcon : alarmOffIcon, 8, 8, SSD1306_WHITE); // Uses a ternary operator to select the correct icon | Allows us to avoid an if statement

  display.drawLine(0, 15, 128, 15, SSD1306_WHITE);
}

void updateFanSpeed()
{
  if (fanSpeed >= 100)
  {
    analogWrite(MOTOR_PIN, 255);
    return;
  }
  if (fanSpeed <= 0)
  {
    analogWrite(MOTOR_PIN, 0);
    return;
  }
  int pulseWidth = map(fanSpeed, 5, 100, 38, 255);
  analogWrite(MOTOR_PIN, pulseWidth);
}

void updateTemperature()
{
  // Get the voltage reading from the TMP36
  int reading = analogRead(temperaturePin);

  // Convert that reading into voltage
  // Replace 5.0 with 3.3, if using a 3.3V Arduino
  float voltage = reading * (5.0 / 1024.0);

  float temperatureC = (voltage - 0.5) * 100;

  // Converts the temperature to F
  int temperatureHolder = (temperatureC * 9.0 / 5.0) + 32.0;
  if (temperatureHolder > 0)
    temperatureF = temperatureHolder;
}

void setAlarmMenu()
{
  int xOffset;
  switch (menuSelectionIndex)
  {
  case 0: // Shows the currect alarm time
    switch (pressedButton)
    {
    case -2:
      display.clearDisplay();
      display.setTextSize(1);
      displayCenteredText("Alarm Set For", 0);
      display.drawLine(25, 8, 101, 8, SSD1306_WHITE);
      display.setTextSize(2);
      // displayCenteredText((String(alarmHour) + ":" + String(alarmMinute) + "\n"), 23);
      char timeText[9]; // Buffer for "HH:MM:SS\0"
      sprintf(timeText, "%02i:%02i", alarmHour, alarmMinute);
      displayCenteredText(String(timeText), 23);

      display.setTextSize(1);
      display.setCursor(5, 56);
      display.print("Press ");
      display.drawBitmap(39, 55, playIcon, 8, 8, SSD1306_WHITE);
      display.print(" To Edit Alarm");
      display.display();
      break;
    case 14: // The Back Button directly below Menu
      menuSelectionIndex = 0;
      menu = 5;
      break;
    case 9:
    case 1: // The Play Button directly below Plus
      menuSelectionIndex = 1;
      subMenuSelectionIndex = 0;
      break;
    }
    break;
  case 1: // Edit Alarm
    switch (pressedButton)
    {
    case -2:
      display.clearDisplay();
      display.setTextSize(1);
      displayCenteredText("Set Alarm Time", 0);
      display.setTextSize(2);

      char alarmTimeText[7]; // Buffer for "HH:MM\0"
      sprintf(alarmTimeText, "%02d:%02d", alarmHour, alarmMinute);
      displayCenteredText(String(alarmTimeText), 18);

      // Uses a ternary and the subMenuSelectionIndex to determine the x positions of the line
      xOffset = (subMenuSelectionIndex < 2) ? (34 + (subMenuSelectionIndex * 12)) : (82 + ((subMenuSelectionIndex - 3) * 12));
      display.drawLine(xOffset, 33, xOffset + 9, 33, SSD1306_WHITE);

      display.setTextSize(1);
      displayCenteredText("Press  To Save", 40);
      display.drawBitmap(56, 39, playIcon, 8, 8, SSD1306_WHITE);
      displayCenteredText("Press   To Cancel", 50);
      display.drawBitmap(48, 49, backIcon, 8, 8, SSD1306_WHITE);

      display.display();
      break;
    case 8: // Left Button
      subMenuSelectionIndex = (subMenuSelectionIndex + 3) % 4;
      break;
    case 10: // Right Button
      subMenuSelectionIndex = (subMenuSelectionIndex + 1) % 4;
      break;
    case 9:
    case 1: // The Play Button directly below Plus
      // Alarm.free(mainAlarm);
      // mainAlarm = Alarm.alarmRepeat(alarmHour, alarmMinute, 0, triggerAlarm);
      prefs.alarmHourEEPROM = alarmHour;
      prefs.alarmMinuteEEPROM = alarmMinute;
      savePreferences();
      // int rc = myFlashPrefs.readPrefs(&prefs, sizeof(prefs));
      // menuSelectionIndex = 0;
      Serial.print("Alarm saved and set for: ");
      Serial.print(alarmHour);
      Serial.print(":");
      Serial.println(alarmMinute);
      restartProgram();
      break;
    case 14: // The Back Button directly below Menu
      alarmHour = prefs.alarmHourEEPROM;
      alarmMinute = prefs.alarmMinuteEEPROM;
      menuSelectionIndex = 0;
      break;
    default: // A Number Key was probably pressed
      // Convert the pressedButton code from it's IR code the the keypad number
      int pressedNumber = 0;
      switch (pressedButton)
      {
      case 16:
        pressedNumber = 1;
        break;
      case 17:
        pressedNumber = 2;
        break;
      case 18:
        pressedNumber = 3;
        break;
      case 20:
        pressedNumber = 4;
        break;
      case 21:
        pressedNumber = 5;
        break;
      case 22:
        pressedNumber = 6;
        break;
      case 24:
        pressedNumber = 7;
        break;
      case 25:
        pressedNumber = 8;
        break;
      case 26:
        pressedNumber = 9;
        break;
      case 12:
        pressedNumber = 0;
        break;
      }
      switch (subMenuSelectionIndex)
      {
      case 0: // Tens Digit of alarmHour
        alarmHour = (alarmHour % 10) + ((pressedNumber % 3) * 10);
        if (alarmHour > 23)
          alarmHour = 23;
        break;
      case 1: // Ones Digit of alarmHour
        alarmHour = (alarmHour / 10) * 10 + pressedNumber;
        if (alarmHour > 23)
          alarmHour = 23;
        break;
      case 2: // Tens Digit of alarmMinute
        alarmMinute = (alarmMinute % 10) + ((pressedNumber % 6) * 10);
        if (alarmMinute > 59)
          alarmMinute = 59;
        break;
      case 3: // Ones Digit of alarmMinute
        alarmMinute = (alarmMinute / 10) * 10 + pressedNumber;
        if (alarmMinute > 59)
          alarmMinute = 59;
        break;
      }
      subMenuSelectionIndex = (subMenuSelectionIndex + 1) % 4;
      break;
    }
    break;
  }
}


void setAlarmSound()
{
  switch (pressedButton)
  {
  case 8:  // The left button (Directly to the left of play)
  case 5:  // The plus button
    menuSelectionIndex = (menuSelectionIndex + TOTAL_SONGS - 1) % TOTAL_SONGS;
    
    // Adjust scroll offset if selection is above visible area
    if (menuSelectionIndex < scrollOffset) {
      scrollOffset = menuSelectionIndex;
    }
    // Adjust scroll offset if selection is below visible area
    else if (menuSelectionIndex >= scrollOffset + MAX_VISIBLE_ITEMS) {
      scrollOffset = menuSelectionIndex - MAX_VISIBLE_ITEMS + 1;
    }
    
    resetSongs();
    break;
    
  case 10: // The next button (Directly to the right of play)
  case 13: // The Minute Button
    menuSelectionIndex = (menuSelectionIndex + 1) % TOTAL_SONGS;
    
    // Adjust scroll offset if selection is below visible area
    if (menuSelectionIndex >= scrollOffset + MAX_VISIBLE_ITEMS) {
      scrollOffset = menuSelectionIndex - MAX_VISIBLE_ITEMS + 1;
    }
    // Adjust scroll offset if selection is above visible area
    else if (menuSelectionIndex < scrollOffset) {
      scrollOffset = menuSelectionIndex;
    }
    
    resetSongs();
    break;
    
  case 9:  // Select button
    menu = 5;
    alarmSong = menuSelectionIndex;
    prefs.alarmSong = alarmSong;
    prefs.scrollOffset = scrollOffset;
    savePreferences();
    menuSelectionIndex = 0;
    scrollOffset = 0; // Reset scroll when leaving menu
    break;
    
  case 14: // Back button
    menu = 5;
    menuSelectionIndex = 0;
    scrollOffset = 0; // Reset scroll when leaving menu
    break;
    
  default: // Draw the menu options
    display.clearDisplay();
    display.setTextSize(1);
    displayCenteredText("Set Alarm Sound", 0);

    // Array of song names for easier management
    const char* songs[TOTAL_SONGS] = {
      "Sound 1",
      "Sound 2",
      "Paris",
      "Overtime",
      "No Role Modelz",
      "Dies Irae",
      "Highest In the Room"
    };

    // Display only the visible items
    for (int i = 0; i < MAX_VISIBLE_ITEMS; i++) {
      int songIndex = i + scrollOffset;
      if (songIndex < TOTAL_SONGS) {
        display.setCursor(8, 18 + i * 10);
        display.println(songs[songIndex]);
        
        // Show selection indicator
        if (songIndex == menuSelectionIndex) {
          display.setCursor(0, 18 + i * 10);
          display.println(">");
        }
      }
    }

    // Show scroll indicators if there are more items above or below
    if (scrollOffset > 0) {
      display.setCursor(120, 22);
      display.println("^"); // Up arrow
    }
    if (scrollOffset + MAX_VISIBLE_ITEMS < TOTAL_SONGS) {
      display.setCursor(120, 42);
      display.println("v"); // Down arrow
    }

    display.display();
    songLoop(menuSelectionIndex);
    break;
  }
}

void setTimeMenu()
{
  int xOffset;
  switch (menuSelectionIndex)
  {
  case 0: // Shows the currect system time
    switch (pressedButton)
    {
    case -2:
      display.clearDisplay();
      display.setTextSize(1);
      displayCenteredText("Time Set To", 0);
      display.drawLine(25, 8, 101, 8, SSD1306_WHITE);
      display.setTextSize(2);
      char timeText[11]; // Buffer for "HH:MM:SS\0"
      sprintf(timeText, "%02i:%02i:%02i", hour(), minute(), second());
      displayCenteredText(String(timeText), 23);

      display.setTextSize(1);
      display.setCursor(5, 56);
      display.print("Press ");
      display.drawBitmap(39, 55, playIcon, 8, 8, SSD1306_WHITE);
      display.print(" To Edit Time");
      display.display();
      break;
    case 14: // The Back Button directly below Menu
      menuSelectionIndex = 0;
      menu = 5;
      break;
    case 9:
    case 1: // The Play Button directly below Plus
      menuSelectionIndex = 1;
      subMenuSelectionIndex = 0;
      tempTimeHour = hour();
      tempTimeMinute = minute();
      tempTimeSecond = second();
      break;
    }
    break;
  case 1: // Edit Time
    switch (pressedButton)
    {
    case -2:
      display.clearDisplay();
      display.setTextSize(1);
      displayCenteredText("Set Current Time", 0);
      display.setTextSize(2);

      char currentTimeText[9]; // Buffer for "HH:MM:SS\0"
      sprintf(currentTimeText, "%02d:%02d:%02d", tempTimeHour, tempTimeMinute, tempTimeSecond);
      displayCenteredText(String(currentTimeText), 18);

      // Adjust to make the underline look good
      int xOffset;
      switch (subMenuSelectionIndex)
      {
      case 0:
        xOffset = 16;
        break;
      case 1:
        xOffset = 28;
        break;
      case 2:
        xOffset = 52;
        break;
      case 3:
        xOffset = 64;
        break;
      case 4:
        xOffset = 88;
        break;
      case 5:
        xOffset = 100;
        break;
      }
      Serial.print("xOffset: ");
      Serial.println(xOffset);
      display.drawLine(xOffset, 33, xOffset + 9, 33, SSD1306_WHITE);

      display.setTextSize(1);
      displayCenteredText("Press  To Save", 43);
      display.drawBitmap(56, 42, playIcon, 8, 8, SSD1306_WHITE);
      displayCenteredText("Press   To Cancel", 53);
      display.drawBitmap(48, 52, backIcon, 8, 8, SSD1306_WHITE);

      display.display();
      break;
    case 8: // Left Button
      subMenuSelectionIndex = (subMenuSelectionIndex + 5) % 6;
      break;
    case 10: // Right Button
      subMenuSelectionIndex = (subMenuSelectionIndex + 1) % 6;
      break;
    case 9:
    case 1:                                                                            // The Play Button directly below Plus
      rtc.adjust(DateTime(2025, 2, 20, tempTimeHour, tempTimeMinute, tempTimeSecond)); // Change the RTC time
      restartProgram();                                                                // We can't update now so we are restarting the fan
      break;
    case 14: // The Back Button directly below Menu
      menuSelectionIndex = 0;
      break;
    default: // A Number Key was probably pressed
      // Convert the pressedButton code from it's IR code the the keypad number
      int pressedNumber = 0;
      switch (pressedButton)
      {
      case 16:
        pressedNumber = 1;
        break;
      case 17:
        pressedNumber = 2;
        break;
      case 18:
        pressedNumber = 3;
        break;
      case 20:
        pressedNumber = 4;
        break;
      case 21:
        pressedNumber = 5;
        break;
      case 22:
        pressedNumber = 6;
        break;
      case 24:
        pressedNumber = 7;
        break;
      case 25:
        pressedNumber = 8;
        break;
      case 26:
        pressedNumber = 9;
        break;
      case 12:
        pressedNumber = 0;
        break;
      }
      switch (subMenuSelectionIndex)
      {
      case 0: // Tens Digit of tempTimeHour
        tempTimeHour = (tempTimeHour % 10) + ((pressedNumber % 3) * 10);
        if (tempTimeHour > 23)
          tempTimeHour = 23;
        break;
      case 1: // Ones Digit of tempTimeHour
        tempTimeHour = (tempTimeHour / 10) * 10 + pressedNumber;
        if (tempTimeHour > 23)
          tempTimeHour = 23;
        break;
      case 2: // Tens Digit of tempTimeMinute
        tempTimeMinute = (tempTimeMinute % 10) + ((pressedNumber % 6) * 10);
        if (tempTimeMinute > 59)
          tempTimeMinute = 59;
        break;
      case 3: // Ones Digit of tempTimeMinute
        tempTimeMinute = (tempTimeMinute / 10) * 10 + pressedNumber;
        if (tempTimeMinute > 59)
          tempTimeMinute = 59;
        break;
      case 4: // Tens Digit of tempTimeSecond
        tempTimeSecond = (tempTimeSecond % 10) + ((pressedNumber % 6) * 10);
        if (tempTimeSecond > 59)
          tempTimeSecond = 59;
        break;
      case 5: // Ones Digit of tempTimeSecond
        tempTimeSecond = (tempTimeSecond / 10) * 10 + pressedNumber;
        if (tempTimeSecond > 59)
          tempTimeSecond = 59;
      }
      subMenuSelectionIndex = (subMenuSelectionIndex + 1) % 6;
      break;
    }
    break;
  }
}

uint32_t wheel(byte pos)
{ // used for the rainbow neoPixel effect
  pos = 255 - pos;
  if (pos < 85)
  {
    return ring.Color(255 - pos * 3, 0, pos * 3);
  }
  else if (pos < 170)
  {
    pos -= 85;
    return ring.Color(0, pos * 3, 255 - pos * 3);
  }
  else
  {
    pos -= 170;
    return ring.Color(pos * 3, 255 - pos * 3, 0);
  }
}

void animateNeoPixel()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastNeoUpdate > neoWaitTime)
  { // Adjust timing for different effects
    lastNeoUpdate = currentMillis;
    if (neoColor == 8)
    {
      ring.clear(); // Turns off the LEDs
      ring.show();
      return; // Prevents the for loop after from running
    }
    switch (neoPixelMode)
    {
    case 0: // Wipe effect
      ring.clear();
      updatePixelWithColor(neoColor, neoPixelIndex);
      ring.show();
      neoPixelIndex = (neoPixelIndex + 1) % (NUM_LEDS - 4);
      break;
    case 1: // Cycle
      if (neoPixelIndex < NUM_LEDS - 4)
      {
        updatePixelWithColor(neoColor, neoPixelIndex); // Sets the color of the pixel
      }
      else
      {
        ring.setPixelColor(neoPixelIndex % (NUM_LEDS - 4), 0, 0, 0); // Sets the color of the pixel to black (off)
      }
      ring.show();
      neoPixelIndex = (neoPixelIndex + 1) % ((NUM_LEDS - 4) * 2);
      break;

    case 2: // Breathing effect
      neoBrightness += neoPixelFadeDirection * 1;
      if (neoBrightness >= maxNeoBrightness || neoBrightness <= 1)
      {
        neoBrightness = neoBrightness <= 1 ? 1 : maxNeoBrightness; // Helps clamp the brightness
        neoPixelFadeDirection *= -1;
      }
      if (neoColor == 7) // Rainbow pattern
      {
        // Apply brightness scaling while maintaining color ratios
        for (int i = 0; i < NUM_LEDS; i++)
        {
          uint32_t color = wheel((i * 256 / NUM_LEDS) & 255);

          // Extract RGB components
          uint8_t r = (color >> 16) & 0xFF;
          uint8_t g = (color >> 8) & 0xFF;
          uint8_t b = color & 0xFF;

          // Scale with brightness while maintaining color ratios
          r = (r * neoBrightness) / maxNeoBrightness;
          g = (g * neoBrightness) / maxNeoBrightness;
          b = (b * neoBrightness) / maxNeoBrightness;

          // Ensure minimum brightness to maintain color perception
          if (neoBrightness < 20)
          {                     // Adjust this threshold as needed
            uint8_t minVal = 1; // Minimum value to maintain color
            if (r > 0)
              r = max(r, minVal);
            if (g > 0)
              g = max(g, minVal);
            if (b > 0)
              b = max(b, minVal);
          }

          ring.setPixelColor(i, r, g, b);
        }
      }
      else
      {
        // For solid colors, use the standard brightness control
        ring.setBrightness(neoBrightness);
      }
      ring.show();
      break;
    case 3: // solid
      ring.show();
      break; // Does nothing
    }
  }
}

// A function of type int array pointer that returns an array with rgb values
int *calculateColors(int color)
{ // neoColor guide: 0 = all white, 1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = purple, 6 = pink, 7 = rainbow
  // MAKE SURE TO DELETE THIS ALLOCATED ARRAY TO PREVENT MEMORY LEAKS
  int *rgb = new int[3];        // Create an array of size 3 to hold the RGB values
  rgb[0] = rgb[1] = rgb[2] = 0; // Initialize all values to 0
  // figure out the color to generate
  switch (color)
  {
  case 0: // White
    rgb[0] = 255;
    rgb[1] = 255;
    rgb[2] = 255;
    break;
  case 1: // Red
    rgb[0] = 255;
    break;
  case 2: // Green
    rgb[1] = 255;
    break;
  case 3: // Blue
    rgb[2] = 255;
    break;
  case 4: // Yellow
    rgb[0] = 255;
    rgb[1] = 255;
    break;
  case 5: // Purple
    rgb[0] = 255;
    rgb[2] = 255;
    break;
  case 6: // Pink
    rgb[0] = 255;
    rgb[1] = 100;
    rgb[2] = 255;
    break;
  }
  return rgb; // Return the pointer to the array
}

void updatePixelWithColor(int color, int pixel)
{ // neoColor guide: 0 = all white, 1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = purple, 6 = pink, 7 = rainbow
  if (color == 7)
  {
    ring.setPixelColor(pixel, wheel((pixel * 256 / NUM_LEDS) & 255)); // Set the color of each pixel in the ring
    return;
  }
  int *colors = calculateColors(neoColor);
  int red = colors[0];
  int green = colors[1];
  int blue = colors[2];
  delete[] colors;                             // Prevents memory leaks
  ring.setPixelColor(pixel, red, green, blue); // Sets the color
}

void neoModeUpdated()
{ // neoColor guide: 0 = all white, 1 = red, 2 = green, 3 = blue, 4 = yellow, 5 = purple, 6 = pink, 7 = rainbow
  prefs.neoColor = neoColor;
  prefs.neoPixelMode = neoPixelMode;
  prefs.neoBrightness = neoBrightness;
  prefs.neoSpeed = neoSpeed;
  savePreferences();

  neoWaitTime = map(neoSpeed, 0, 100, slowestNeoSpeed, 5); // Maps the speed to a wait time between updates

  ring.setBrightness(neoBrightness); // Set all LEDs to the brightness level
  if (neoColor == 7)
  {
    for (int i = 0; i < NUM_LEDS; i++)
      ring.setPixelColor(i, wheel((i * 256 / NUM_LEDS) & 255)); // Set the color of each pixel in the ring
    return;                                                     // Prevents the for loop after from running
  }

  // figure out the color to generate
  int *colors = calculateColors(neoColor);
  int red = colors[0];
  int green = colors[1];
  int blue = colors[2];
  delete[] colors; // Prevents memory leaks
  for (int i = 0; i < NUM_LEDS; i++)
    ring.setPixelColor(i, red, green, blue); // Sets the color
}

void neoPixelConfigMenu()
{
  return;
}

void displayCenteredText(String text, int height)
{
  int16_t x1, y1;                 // Placeholders for the top left of the text box
  uint16_t textWidth, textHeight; // Will store the text width and height

  // Get text width dynamically
  display.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight); // Built-In function that calculates how much space the text will take on the screen and outputs the data to parameters 3-6

  // Calculate new X position to center the text
  int newCursorX = (SCREEN_WIDTH - textWidth) / 2;

  display.setCursor(newCursorX, height);
  display.print(text);
}

void displayRightAlignedText(String text, int height)
{
  int16_t x1, y1;                 // Placeholders for the top left of the text box
  uint16_t textWidth, textHeight; // Will store the text width and height

  // Get text width dynamically
  display.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight); // Built-In function that calculates how much space the text will take on the screen and outputs the data to parameters 3-6

  // Calculate new X position to center the text
  int newCursorX = SCREEN_WIDTH - textWidth;

  display.setCursor(newCursorX, height);
  display.print(text);
}

void alarmGoingOffMenu()
{
  Serial.println("Alarm Going Off Menu");

  display.clearDisplay();
  display.setTextSize(2);
  char timeText[9]; // Buffer for "HH:MM:SS\0"
  sprintf(timeText, "%02d:%02d:%02d", hour(), minute(), second());
  displayCenteredText(String(timeText), 20);
  display.setTextSize(1);
  displayCenteredText("Press Enter Button", 55);
  display.display();

  // The modulus determines how much time is between the start of each tone
  // The < determines how long the tone plays during each interval
  if (((millis() - alarmWentOffTime) % 10000) < 5000)
  {
    fanSpeed = 100; // Fan on full blast;
  }
  else
  {
    fanSpeed = 0; // Fan off
  }
  updateFanSpeed();

  songLoop(alarmSong);

  if (pressedButton == 9)
  { // Stop the alarm if the physical button is pressed or if any button on the remote is pressed
    Serial.println("Button Pressed");
    menu = 0;
    menuSelectionIndex = 0;
    alarmGoingOff = false;
    Alarm.enable(mainAlarm); // Re-enable the alarm
    fanSpeed = prefs.fanSpeed;
    updateFanSpeed();
  }

  return;
}

void menuSelectionMenu()
{
  switch (pressedButton)
  {
  case 8: // The left button (Directly to the left of play)
    menuSelectionIndex = (menuSelectionIndex + 3) % 4;
    break;
  case 5: // The plus button
    menuSelectionIndex = (menuSelectionIndex + 3) % 4;
    break;
  case 10: // The next button (Directly to the right of play)
    menuSelectionIndex = (menuSelectionIndex + 1) % 4;
    break;
  case 13: // The Minute Button
    menuSelectionIndex = (menuSelectionIndex + 1) % 4;
    break;
  case 9:
    // if (menuSelectionIndex == 4) resetEEPROM();
    pressedButton = -2; // Prevents the button from being pressed again
    menu = menuSelectionIndex + 1; // Menu Selection Index is the currently selected menu
    Serial.print("Selected Menu: ");
    Serial.println(menu);
    menuSelectionIndex = 0;
    if (menu == 2){ // Set alarm sound menu
      loadPreferences();
      menuSelectionIndex = prefs.alarmSong;
      scrollOffset = prefs.scrollOffset;
      resetSongs();
    }
    break;
  case 14:
    menu = 0;
    menuSelectionIndex = 0;
    break;
  default: // Draws the menu options
    display.clearDisplay();
    drawDisplayHeader();
    display.setTextSize(1);
    display.setCursor(8, 16);
    display.println("Alarm Time");
    display.setCursor(8, 26);
    display.println("Alarm Sound");
    display.setCursor(8, 36);
    display.println("Set Current Time");
    display.setCursor(8, 46);
    display.println("Reset EEPROM");

    display.setCursor(0, 10 * menuSelectionIndex + 16);
    display.println(">");

    display.display();
    break;
  }
}

void triggerAlarm()
{
  Serial.println("Alarm Triggered Method Ran");
  alarmJustTriggered = true;
  resetSongs();
}

void restartProgram()
{
  NVIC_SystemReset(); // Triggers a system reset on ARM Cortex-M4
}

void loadPreferences()
{
  EEPROM.get(0, prefs);
}

void savePreferences()
{
  EEPROM.put(0, prefs);
}

void resetEEPROM()
{
  prefs.alarmHourEEPROM = 12;
  prefs.alarmMinuteEEPROM = 30;
  prefs.alarmSetBeforeEEPROM = 91;
  prefs.fanSpeed = 0;
  prefs.alarmSong = 0;
  prefs.alarmEnabled = true;
  prefs.alarmSetBeforeEEPROM = 0;
  prefs.neoColor = 0;
  prefs.neoPixelMode = 0;
  prefs.neoBrightness = 25;
  prefs.neoSpeed = 70;
  savePreferences();
  restartProgram();
}
// DO NOT RUN THIS STUPID CODE

// void nuclearResetFlash() {
//   // Initialize the flash interface
//   mbed::FlashIAP flash;
//   flash.init();

//   // Get the flash info
//   uint32_t flashStartAddress = flash.get_flash_start();
//   uint32_t flashSize = flash.get_flash_size();

//   // Erase the entire flash (use with caution!)
//   flash.erase(flashStartAddress, flashSize);
//   flash.deinit();

//   delay(1000);

//   // Force a reboot to apply changes
//   NVIC_SystemReset();
// }