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
#include "vent_control_relay.h"
#include "vent_control_settings.h"
#include "vent_control_timer.h"
#include "vent_control_EEPROM.h"

#include <Arduino.h>

RelayInfo g_relay[8] = {{0},{1},{2},{3}, {4},{5},{6},{7}};

RelayInfo::RelayInfo(byte index) {
  this->index = index;
  cycles = 0;
  exhaling = false;
  nextToggle = 250 * (int)index;
  onToExhale = true;
  pin = index + 3;   // Could make this a lookup table.
  pinMode(pin, OUTPUT);
  setRelay();
}

void RelayInfo::setRelay() {
  digitalWrite(pin, (exhaling != onToExhale) ? HIGH : LOW);
}

unsigned long RelayInfo::getCycles(void) {
  return cycles;
}

void RelayInfo::setCycles(unsigned long cycles) {
  this->cycles = cycles;
  scheduleEEPROMUpdate();
}

unsigned long RelayInfo::incrementCycles(unsigned long offset) {
  cycles += offset;
  return cycles;
}

void RelayInfo::resetCycles(void) {
  Serial.println("Reset Relay Count for patient "); Serial.println(index + 1);
  Serial.println("Previous Relay Count was "); Serial.println(cycles);
  
  this->setCycles(0);
}

void RelayInfo::updateRelay(unsigned long now) {
  int newExhaling = exhaling;
  if (nextToggle < now) {
    if (getSettings(index)->enabled) {
      newExhaling = !newExhaling;
    } else {
      newExhaling = false;                // When disabled, set the timer to inhale
    }
  }
  if (newExhaling != exhaling) {
    delay(4);  // Delay 4 ms so not all the relays bang at once

    if (newExhaling) {
      incrementCycles(1);
    }
    VentControlSettings *settings = (VentControlSettings *) getSettings(index);
    if (newExhaling) {
      duration = settings->getExhaleDurationInMS();
    } else {
      duration = settings->getInhaleDurationInMS();
    }
    nextToggle = now + duration; // XXX (g_relayState[i] ? g_exhale_ds[i] : g_inhale_ds[i]) * 100;
    exhaling = newExhaling;
    Serial.print("Relay "); Serial.print(index+1); Serial.print(exhaling ? ": EX" : ": IN"); Serial.print(" for "); Serial.print(duration); Serial.println(" ms.");
    setRelay();
  }
}

void updateRelays(unsigned long now) {
  for (int i = 0; i < 8; i++) {
    //g_relay[i].setIndex(i);
    g_relay[i].updateRelay(now);
  }
}
