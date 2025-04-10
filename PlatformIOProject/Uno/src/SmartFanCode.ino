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
  bool alarmEnabled;
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
int neoPixelIndex = 1;
int neoPixelBrightness = 255;
int neoPixelFadeDirection = -1;
float temperatureF;

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
    prefs.alarmEnabled = true;
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
  }
  mainAlarm = Alarm.alarmRepeat(alarmHour, alarmMinute, 0, triggerAlarm); // Alarm set for alarmHour:alarmMinute:00 | Calls the triggerAlarm method

  pinMode(MOTOR_PIN, OUTPUT);
  updateFanSpeed();

  NoteIndex = 0;

  Serial.println("Setup Complete");
}

void loop()
{
  Alarm.delay(0); // Needed to refresh the time in the TimeAlarms library
  checkForIR();

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
  case 4:
    neoPixelConfigMenu();
    break;
  case 5:
    menuSelectionMenu();
    break;
  case 6:
    alarmGoingOffMenu();
    break;
  }

  animateNeoPixel();
}

void mainMenu()
{
  switch (pressedButton)
  {
  case 4:
    menu = 5;
    menuSelectionIndex = 0;
    break;
  case 6:
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
  case 25:
    restartProgram();
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
  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
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
  case 8: // The left button (Directly to the left of play)
    menuSelectionIndex = (menuSelectionIndex + 3) % 2;
    resetSongs();
    break;
  case 5: // The plus button
    menuSelectionIndex = (menuSelectionIndex + 3) % 2;
    resetSongs();
    break;
  case 10: // The next button (Directly to the right of play)
    menuSelectionIndex = (menuSelectionIndex + 1) % 2;
    resetSongs();
    break;
  case 13: // The Minute Button
    menuSelectionIndex = (menuSelectionIndex + 1) % 2;
    resetSongs();
    break;
  case 9:
    menu = 5; // Menu Selection Index is the currently selected menu
    alarmSong = menuSelectionIndex;
    prefs.alarmSong = alarmSong;
    savePreferences();
    menuSelectionIndex = 0;
    break;
  case 14:
    menu = 5;
    menuSelectionIndex = 0;
    break;
  default: // Draws the menu options
    display.clearDisplay();

    display.setTextSize(1);
    displayCenteredText("Set Alarm Sound", 0);

    display.setCursor(8, 18);
    display.println("Sound 1");
    display.setCursor(8, 28);
    display.println("Sound 2");

    display.setCursor(0, 10 * menuSelectionIndex + 18);
    display.println(">");

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

  if (currentMillis - lastNeoUpdate > 50)
  { // Adjust timing for different effects
    lastNeoUpdate = currentMillis;
    switch (menu)
    {
    case 2:
      Serial.println("Animate the Neopixel for fan speed");
      break;
    default:
      switch (neoPixelMode)
      {
      case 0: // Wipe effect
        ring.clear();
        ring.setPixelColor(neoPixelIndex, wheel((neoPixelIndex * 256 / NUM_LEDS) & 255));
        ring.show();
        neoPixelIndex = (neoPixelIndex + 1) % NUM_LEDS;
        if (neoPixelIndex == 1)
        {
          neoPixelMode = 1;
        }
        break;

      case 1: // Rainbow cycle
        ring.setPixelColor(neoPixelIndex, wheel((neoPixelIndex * 256 / NUM_LEDS) & 255));
        ring.show();
        neoPixelIndex = (neoPixelIndex + 1) % NUM_LEDS;
        if (neoPixelIndex == 0)
          neoPixelMode = 2;
        break;

      case 2: // Breathing effect
        neoPixelBrightness += neoPixelFadeDirection * 10;
        if (neoPixelBrightness >= 255 || neoPixelBrightness <= 10)
        {
          neoPixelFadeDirection *= -1;
        }
        ring.setBrightness(neoPixelBrightness); // Set all LEDs to the brightness level
        ring.show();

        if (neoPixelBrightness >= 255)
        {
          neoPixelBrightness = 255;
          ring.setBrightness(255);
          neoPixelMode = 0;
          neoPixelIndex = 1;
        }
        break;
      }
      break;
    }
  }
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
    menuSelectionIndex = (menuSelectionIndex + 4) % 5;
    break;
  case 5: // The plus button
    menuSelectionIndex = (menuSelectionIndex + 4) % 5;
    break;
  case 10: // The next button (Directly to the right of play)
    menuSelectionIndex = (menuSelectionIndex + 1) % 5;
    break;
  case 13: // The Minute Button
    menuSelectionIndex = (menuSelectionIndex + 1) % 5;
    break;
  case 9:
    if (menuSelectionIndex == 4) resetEEPROM();

    menu = menuSelectionIndex + 1; // Menu Selection Index is the currently selected menu
    Serial.print("Selected Menu: ");
    Serial.println(menu);
    menuSelectionIndex = 0;
    if (menu == 2)
    {
      menuSelectionIndex = alarmSong;
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
    display.println("Configure NeoPixel");
    display.setCursor(8, 56);
    display.println("Reset Save-Data");

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


void resetEEPROM(){
  prefs.alarmSetBeforeEEPROM = 0;
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