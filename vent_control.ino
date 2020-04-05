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

#include "vent_control_config.h"
#include "vent_control_globals.h"
#include "vent_control_display.h"
#include "vent_control_input.h"
#include "vent_control_relay.h"
#include "vent_control_timer.h"
#include "vent_control_settings.h"
#include "vent_control_EEPROM.h"
#include <Arduino.h>

unsigned long g_startTime;

void setup() {
  const __FlashStringHelper *line = F("======================================================================");

  Serial.begin(g_serialSpeed);
  Serial.println(line);  
  Serial.println(F("= Copyright 2020, Jim Sokoloff                                       ="));  
  Serial.println(F("= This code is licensed under the Apache 2.0 open source license.    ="));  
  Serial.println(F("= https://github.com/jim-sokoloff for updates, requests, or features ="));  
  Serial.println(F("=                                                                    ="));  
  Serial.println(F("= It is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR       ="));  
  Serial.println(F("= CONDITIONS OF ANY KIND, either express or implied.                 ="));  
  Serial.println(F("=                                                                    ="));  
#if LIMITED_MEMORY
  Serial.println(F("= Thank you to all medical staff!                                    ="));
  Serial.println(line);
#else
  Serial.println(line);
  Serial.println(F("Hello there."));
  Serial.println(F("If you're using this to try to save lives, things"));
  Serial.println(F("have gone terribly wrong."));
  Serial.println(F(""));
  Serial.println(F("Our sympathies and deepest respects go to you"));
  Serial.println(F("and your colleagues."));
  Serial.println(F(""));
  Serial.println(F("Thank you for all of what you all have already done"));
  Serial.println(F("and what you are about to do."));
  Serial.println(F(""));
  Serial.println(F("Here's hoping for the best possible outcome for you,"));
  Serial.println(F("all your colleagues, and all of your patients."));
  Serial.println(F(""));
  Serial.println(F("Sincerely,"));
  Serial.println(F("The team."));
  Serial.println(line);  
#endif
  
  g_startTime = millis();

  if (!readEEPROM()) {
    Serial.println(F("Couldn't read configuration from EEPROM."));
    Serial.println(F("Initializing new data."));
    Serial.println(F("This is normal for the first usage."));
    updateEEPROM(millis(), true);
  }
  scheduleEEPROMUpdate( 1UL * 60UL * 1000UL);
}

void loop() {
  unsigned long now = millis();
  
  updateDisplay(now);
  handleButtons(now);
  updateRelays(now);
  updateEEPROM(now);
  
  unsigned long now2 = millis();
  unsigned long delayAmount = 20 - now2 % 20UL;
  delay(delayAmount);
}
