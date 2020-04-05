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

#include <Arduino.h>
#include <EEPROM.h>

#include "vent_control_EEPROM.h"
#include "vent_control_settings.h"
#include "vent_control_relay.h"

unsigned long g_EEPROMupdateTimer = 0;

const byte MAGIC_BYTE_1 = 'J';
const byte MAGIC_BYTE_2 = 'S';
const byte MAGIC_BYTE_3 = 2;

bool readEEPROM() {
  int address = 0;
#if DEBUG
  unsigned long start = millis();
#endif

  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3))
    return false;

  address = 3;
  if (EEPROM.read(address++) != 8)
    return false;
    
  for (int i = 0; i < 8; i++) {
    if (EEPROM.read(address++) != i + 1)
      return false;
    byte enabled = EEPROM.read(address++);
    byte ieRatioIndex = EEPROM.read(address++);
    int breathInTenthsPerMinute = EEPROM.read(address++) + 100;
    ((VentControlSettings *)getSettings(i))->setEnabled(enabled);
    ((VentControlSettings *)getSettings(i))->selectIndex(ieRatioIndex);
    ((VentControlSettings *)getSettings(i))->setBPM10ths(breathInTenthsPerMinute);
    unsigned long cycles = 0;
    unsigned long multiplier = 1;
    for (int j = 0; j < 8; j++) {
      cycles += EEPROM.read(address++) * multiplier;
      multiplier *= 256;
    }
    g_relay[i].setCycles(cycles);
    ((VentControlSettings *)getSettings(i))->checkSettings();
  }

  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3))
    return false;

  address += 3;
#if DEBUG
  Serial.print("Read EEPROM; took "); Serial.print(millis()-start); Serial.print(" ms to read "); Serial.print(address); Serial.println(" bytes. SUCCESS.");
#endif

  return true;
}

void updateEEPROM(unsigned long now, bool force) {
  int address = 0;
  if ((EEPROM.read(address) != MAGIC_BYTE_1) || (EEPROM.read(address+1) != MAGIC_BYTE_2) || (EEPROM.read(address+2) != MAGIC_BYTE_3)) 
    force = true;

  if (!force && (now < g_EEPROMupdateTimer)) {
    return;
  }
  now = millis();
  scheduleEEPROMUpdate( 5UL * 60UL * 1000UL);                     // Five minutes between updates by default

  EEPROM.update(address++, MAGIC_BYTE_1);
  EEPROM.update(address++, MAGIC_BYTE_2);
  EEPROM.update(address++, MAGIC_BYTE_3);

  EEPROM.update(address++, 8);
  for (int i = 0; i < 8; i++) {
    const VentControlSettings *settings;
    settings = getSettings(i);
    EEPROM.update(address++, i+1);
    EEPROM.update(address++, settings->enabled);
    EEPROM.update(address++, settings->ieRatioIndex);
    EEPROM.update(address++, settings->breathInTenthsPerMinute-100);
    unsigned long count = g_relay[i].getCycles();
    for (int j = 0; j < 8; j++) {
      unsigned char b = count % 256;
      count /= 256;
      EEPROM.update(address++, b);
    }
  }
  EEPROM.update(address++, MAGIC_BYTE_1);
  EEPROM.update(address++, MAGIC_BYTE_2);
  EEPROM.update(address++, MAGIC_BYTE_3);

#if DEBUG
  Serial.print("Updated EEPROM; took "); Serial.print(millis()-now); Serial.print(" ms to write "); Serial.print(address); Serial.println(" bytes.");
#endif
}

void scheduleEEPROMUpdate(unsigned long delay) {
  unsigned long newTimer = millis() + delay;
  
  g_EEPROMupdateTimer = newTimer;
}
