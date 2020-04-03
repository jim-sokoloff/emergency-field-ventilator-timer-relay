/*
   Copyright 2020, Jim Sokoloff

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

const char *g_LEDVersion = "u 0.8.0 ";
const char *g_version = "v0.8.0";
const long g_serialSpeed = 115200;

const char *g_messages[] = { "DEEP    ", "DEEPDEEP", "REspEct ", "For All ", "Medical ", "StaFF,  ", "ThankYou", "For WHAt", "You ARE ", "doing!  ", "LOUE USA", "        ", " SERIAL " };
const int g_numStartupMessages = sizeof(g_messages)/sizeof(g_messages[0]);
const unsigned long g_messageTime = 1200UL;

#include <TM1638.h>
#include <EEPROM.h>

const int relay_power = 4;
const int relay_signal = 2;

const int tm1638_data = 11;
const int tm1638_clock = 12;
const int tm1638_strobe = 13;

bool g_usingKeys = 0;
bool g_oneLED = false;

unsigned long g_startTime;
unsigned long g_lastKey = 0;
unsigned long g_lastAnyKey = 0;
unsigned long g_holdRelayCount = 0;
bool g_holdingForRelayCount = false;
unsigned long g_holdOnOff = 0;
bool g_holdingForOnOff = false;
unsigned long g_holdReset = 0;
bool g_holdingForReset = false;

const unsigned long k_keyDelay = 10000;
const unsigned long k_buttonDelay = 350;

unsigned long g_lastEEPROMUpdate = 0;
const byte MAGIC_BYTE_1 = 'J';
const byte MAGIC_BYTE_2 = 'S';
const byte MAGIC_BYTE_3 = 1;

int g_inhale_ds[8] = {10,10,10,10, 10,10,10,10};
int g_exhale_ds[8] = {20,20,20,20, 20,20,20,20};
unsigned long g_relayCycles[8] = {0,0,0,0, 0,0,0,0};
bool g_enabled[8] = {1,1,1,1, 0,0,0,0};
int g_activePatient = 0;
unsigned long g_timerEnd[8] = {0,0,0,0, 0,0,0,0};
int g_relayState[8] = {0,0,0,0, 0,0,0,0};

TM1638 module1(tm1638_data, tm1638_clock, tm1638_strobe);

void updateEEPROM(unsigned long now, bool force = false);

void setup() {
  // initialize serial communication at 115200 bits per second:
  const __FlashStringHelper *line = F("======================================================================");
  Serial.begin(g_serialSpeed);
  Serial.println(line);  
  Serial.println(F("= Copyright 2020, Jim Sokoloff                                       ="));  
  Serial.println(F("= This code is licensed under the Apache 2.0 open source license.    ="));  
  Serial.println(F("= https://github.com/jim-sokoloff for updates, requests, or features ="));  
  Serial.println(line);  
  Serial.println(F("Hello there."));
  Serial.println(F("If you're using this to try to save lives, things"));
  Serial.println(F("have gone terribly wrong."));
  Serial.println("");
  Serial.println(F("Our sympathies and deepest respects go to you"));
  Serial.println(F("and your colleagues."));
  Serial.println("");
  Serial.println(F("Thank you for all of what you all have already done"));
  Serial.println(F("and what you are about to do."));
  Serial.println("");
  Serial.println(F("Here's hoping for the best possible outcome for you,"));
  Serial.println(F("all your colleagues, and all of your patients."));
  Serial.println("");
  Serial.println(F("Sincerely,"));
  Serial.println(F("The team."));
  Serial.println(line);  
  
  
  pinMode(relay_power, OUTPUT);
  pinMode(relay_signal, OUTPUT);
  digitalWrite(relay_power, HIGH);

  g_startTime = millis();
  g_lastKey = g_startTime;
  g_lastAnyKey = g_startTime;
  module1.setDisplayToString(" go USA ");

  for (int i = 0; i < 8; i++) {
    pinMode(3 + i, OUTPUT);
  }
  //module1.setDisplayDigit(1,3,1);
  //module1.setDisplayDigit(1,7,1);
  if (!readEEPROM()) {
    Serial.println(F("Couldn't read configuration from EEPROM."));
    Serial.println(F("Initializing new data."));
    Serial.println(F("This is normal for the first usage."));
    updateEEPROM(millis(), true);
  }
}

void displayOnOffLEDs() {
  for (int i = 0; i < 8; i++) {
    module1.setLED(g_relayState[i], i);
  }
}

int handle_buttons(unsigned long now) {
  bool processNewKey = (millis() > g_lastAnyKey + k_buttonDelay);
  
  byte buttons = module1.getButtons();
  unsigned long runningSecs = (millis() - g_startTime) / 1000;
  
  if (processNewKey && buttons != 0) {
    g_lastAnyKey = millis();
    if (buttons & ~0x03)
      g_lastKey = g_lastAnyKey;
    g_usingKeys = true;
  }
  
  // Patient select
  if (processNewKey) {
    if (buttons & 0x01) {
      g_activePatient--;
    }
    if (buttons & 0x02) {
      g_activePatient++;
    }
  }
  g_activePatient = constrain(g_activePatient, 0, 7);

  if (processNewKey) {
    if (buttons & 0x04) {
      g_inhale_ds[g_activePatient] -= 1;
    }
    if (buttons & 0x08) {
      g_inhale_ds[g_activePatient] += 1;
    }
    if (buttons & 0x40) {
      g_exhale_ds[g_activePatient] -= 1;
    }
    if (buttons & 0x80) {
      g_exhale_ds[g_activePatient] += 1;
    }
  }
  // Hold for reset
  if (buttons == 0x30) {
    if (g_holdingForReset) {
      if (g_lastAnyKey > g_holdReset + 5000UL) {
        Serial.println("Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! ");
        for (int i = 0; i < 8; i++) {
          g_relayCycles[i] = 0;
        }
        g_holdingForReset = false;
      }
    } else {
      g_holdingForReset = true;
      g_holdReset = g_lastAnyKey;
    }
  } else {
    g_holdingForReset = false;
  }
  
  if (g_holdingForOnOff) {
    if (buttons == 0x20) {
      if (g_lastAnyKey > g_holdOnOff + 5000UL) {
        Serial.println("Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! ");
        g_enabled[g_activePatient] = !g_enabled[g_activePatient];
        g_holdingForOnOff = false;
      }
    } else {
      // Released the button early
      g_holdingForOnOff = false;
      g_lastKey = g_startTime;
      g_usingKeys = false;
    }
  } else if (buttons == 0x20) {
    g_holdingForOnOff = true;
    g_holdOnOff = g_lastAnyKey;
  }
  if (g_holdingForRelayCount) {
    if (buttons == 0x10) {
      if (g_lastAnyKey > g_holdRelayCount + 20000UL) {
        Serial.println("Reset 1! Reset 1! Reset 1! Reset 1! Reset 1! Reset 1! Reset 1! Reset 1! Reset 1! ");
        g_relayCycles[g_activePatient] = 0;
        g_holdingForRelayCount = false;
      }
    } else {
      // Released the button early
      g_holdingForRelayCount = false;
      g_lastKey = g_startTime;
      g_usingKeys = false;
    }
  } else if (buttons == 0x10) {
    g_holdingForRelayCount = true;
    g_holdRelayCount = g_lastAnyKey;
  }

  g_inhale_ds[g_activePatient] = constrain(g_inhale_ds[g_activePatient], 4, 40);
  g_exhale_ds[g_activePatient] = constrain(g_exhale_ds[g_activePatient], 5, 50);

  if (g_usingKeys || (buttons != 0)) {
    module1.clearDisplay();
  
    if (g_holdingForReset) {
      unsigned long holdTime = 50 - (now - g_holdReset) / 100;
      module1.setDisplayToString(" Reset");
      displayDS(holdTime, 6);
    } else if (g_holdingForOnOff) {
      unsigned long holdTime = 50 - (now - g_holdOnOff) / 100;
      //Serial.println(holdTime);
      if (holdTime % 10 < 5) {
        module1.setDisplayToString(" Hold ");
      } else {
        if (g_enabled[g_activePatient]) {
          module1.setDisplayToString("  OFF ");
        } else {
          module1.setDisplayToString("  On  ");
        }
      }
      displayDS(holdTime, 6);
      displayOnOffLEDs();
      module1.setLED(millis() % 4 < 2, g_activePatient);
    } else if (g_holdingForRelayCount) {
      if (g_relayCycles[g_activePatient] < 10000000) {
        byte dots = 0;
        if (g_relayCycles[g_activePatient] > 999)
          dots = 0x08;
        if (g_relayCycles[g_activePatient] > 999999)
          dots = 0x48;
        module1.setDisplayToDecNumber(g_relayCycles[g_activePatient],dots,0);
      } else {
        unsigned long cycles = g_relayCycles[g_activePatient]/10000;
        int dot = 2;
        while (cycles > 9999) {
          dot--;
          cycles /= 10;
        }
        for (int i = 0; i < 4; i++) {
          module1.setDisplayDigit(cycles%10, 4-i, i==dot);
          cycles /= 10;
        }
        //module1.setDisplayToDecNumber(g_relayCycles[g_activePatient]/10,0x20,0);
        module1.sendChar(5, 0x00, false);
        module1.sendChar(6, 0x79, false);
        module1.sendChar(7, 0x7d, false);
      }
    } else {
      displayDS(g_inhale_ds[g_activePatient], 2);
      displayDS(g_exhale_ds[g_activePatient], 6);
    }
    module1.setDisplayDigit(g_activePatient+1, 0, 0);
  }
  return (buttons != 0);
}

int displayDS (int ds, int digit) {
  module1.setDisplayDigit(ds/10, digit, 1);
  module1.setDisplayDigit(ds%10, digit+1, 0); 
}

void updateRelays(unsigned long now) { 
  for (int i = 0; i < 8; i++) {
    int state = g_relayState[i];
    if (g_timerEnd[i] < now) {
      if (g_enabled[i]) {
        state = !state;
      } else {
        state = false;
      }
    }
    digitalWrite(i+3, !state);                      // Invert the output so the lights match up (to not confuse people)
    if (state != g_relayState[i]) {
      delay(9);  // Delay 9 ms so not all the relays bang at once
      if (state)
        g_relayCycles[i]++;
      g_timerEnd[i] = now + (g_relayState[i] ? g_exhale_ds[i] : g_inhale_ds[i]) * 100;
      g_relayState[i] = state;
      Serial.print("Relay "); Serial.print(i+1); Serial.println(state ? ": ON" : ": OFF");
    }
  }
}

bool doInitialDisplay(unsigned long now) {
  int message = now/g_messageTime;

  if (message > g_numStartupMessages + 3)
    return false;
    
  if (message < g_numStartupMessages)
    module1.setDisplayToString(g_messages[message]);
  else if (message == g_numStartupMessages)
    module1.setDisplayToDecNumber(g_serialSpeed, 0, false);
  else
    module1.setDisplayToString(g_LEDVersion);

  return true;
}

void updateDisplay(unsigned long now) {
  bool inhale = g_relayState[g_activePatient];

  if (doInitialDisplay(now)) {
    displayOnOffLEDs();
    return;
  } 

  if (g_enabled[g_activePatient]) {
    if (!g_usingKeys && !g_holdingForRelayCount) {
      module1.clearDisplay();
      if (inhale) {
        module1.setDisplayToString("      In");
        displayDS(g_inhale_ds[g_activePatient], 2);
      } else {
        module1.setDisplayToString("  Out  ");
        displayDS(g_exhale_ds[g_activePatient], 6);
      }
    }
    if (!g_holdingForOnOff) {
      unsigned long ticksLeft = g_timerEnd[g_activePatient] - now;
      unsigned long target = (inhale ? g_inhale_ds[g_activePatient] : g_exhale_ds[g_activePatient]) * 100;
      int leds = (target - ticksLeft) * 9 / target;
      if (!inhale) {
        leds = 9 - leds; 
      }
      for (int i = 0; i < 8; i++) {
        module1.setLED(i < leds, i);
      }
    }
  } else {
    if (!g_holdingForOnOff) {
      module1.setDisplayToString("    OFF ");
    }
    //displayDS(g_inhale_ds[g_activePatient], 1);
    //displayDS(g_exhale_ds[g_activePatient], 6);
    displayOnOffLEDs();
  }
  module1.setDisplayDigit(g_activePatient+1, 0, 0);
}

bool readEEPROM() {
  int address = 0;
  unsigned long start = millis();
  
  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3))
    return false;

  address = 3;
  if (EEPROM.read(address++) != 8)
    return false;
    
  for (int i = 0; i < 8; i++) {
    if (EEPROM.read(address++) != i + 1)
      return false;
    g_enabled[i] = EEPROM.read(address++);
    g_inhale_ds[i] = EEPROM.read(address++);
    g_exhale_ds[i] = EEPROM.read(address++);
    unsigned long cycles = 0;
    unsigned long multiplier = 1;
    for (int j = 0; j < 8; j++) {
      cycles += EEPROM.read(address++) * multiplier;
      multiplier *= 256;
    }
    g_relayCycles[i] = cycles;
  }

  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3))
    return false;

  address +- 3;
  Serial.print("Read EEPROM; took "); Serial.print(millis()-start); Serial.print(" ms to read "); Serial.print(address); Serial.println(" bytes. SUCCESS.");

  return true;
}

void updateEEPROM(unsigned long now, bool force) {
  int address = 0;
  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3)) 
    force = true;

  unsigned long secondsBetweenUpdates = 5 * 60;   // Five minutes between updates by default
  if (g_usingKeys)
    secondsBetweenUpdates = 5;                    // Update every five seconds when the user is interacting with the device
    
  if (!force && (now < g_lastEEPROMUpdate + secondsBetweenUpdates*1000UL)) {
    return;
  }
  g_lastEEPROMUpdate = millis();
  EEPROM.update(address++, MAGIC_BYTE_1);
  EEPROM.update(address++, MAGIC_BYTE_2);
  EEPROM.update(address++, MAGIC_BYTE_3);

  EEPROM.update(address++, 8);
  for (int i = 0; i < 8; i++) {
    EEPROM.update(address++, i+1);
    EEPROM.update(address++, g_enabled[i]);
    EEPROM.update(address++, g_inhale_ds[i]);
    EEPROM.update(address++, g_exhale_ds[i]);
    unsigned long count = g_relayCycles[i];
    for (int j = 0; j < 8; j++) {
      unsigned char b = count % 256;
      count /= 256;
      EEPROM.update(address++, b);
    }
  }
  EEPROM.update(address++, MAGIC_BYTE_1);
  EEPROM.update(address++, MAGIC_BYTE_2);
  EEPROM.update(address++, MAGIC_BYTE_3);

  Serial.print("Updated EEPROM; took "); Serial.print(millis()-g_lastEEPROMUpdate); Serial.print(" ms to write "); Serial.print(address); Serial.println(" bytes.");
}

void loop() {
  // Required to allow us to hot-plug the TM1638 module
  module1.setupDisplay(true, 7);

  unsigned long now = millis();
  
  g_usingKeys = (now < (g_lastKey + k_keyDelay)) && (now > g_messageTime * g_numStartupMessages);

  updateDisplay(now);
  handle_buttons(now);
  updateRelays(now);
  updateEEPROM(now);
  
  unsigned long now2 = millis();
  unsigned long delayAmount = 100 - now2 % 100UL;
  delay(delayAmount);
}
