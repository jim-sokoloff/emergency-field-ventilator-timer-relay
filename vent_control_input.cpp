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
#include "vent_control_input.h"
#include "vent_control_globals.h"
#include "vent_control_display.h"
#include "vent_control_relay.h"
#include "vent_control_settings.h"
#include <Arduino.h>

unsigned long g_lastKey = 0;
unsigned long g_lastAnyKey = 0;

const unsigned long k_keyDelay = 10000;
const unsigned long k_buttonDelay = 250;


byte g_buttons = 0;
unsigned long g_holdRelayCount = 0;
bool g_holdingForRelayCount = false;
unsigned long g_holdOnOff = 0;
bool g_holdingForOnOff = false;
unsigned long g_holdReset = 0;
bool g_holdingForReset = false;


int handleButtons(unsigned long now) {
#if DEBUG
  unsigned long delay = millis()-now;
  if (delay >= 15UL) {
    Serial.print("WARNING: slow (>= 15ms delay): "); Serial.print(delay); Serial.println(" ms in handleButtons.");
  }
#endif
  bool processNewKey = (millis() > g_lastAnyKey + k_buttonDelay);
  
  byte buttons = displayGetButtons();
  // XXX g_buttons = buttons;
  
  if (processNewKey && buttons != 0) {
    g_lastAnyKey = millis();
    if (buttons & ~0x03)
      g_lastKey = g_lastAnyKey;
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
      ((VentControlSettings *)getSettings(g_activePatient))->incrementIERatioIndex(1);
      setDisplayOverride(k_modeIE);
    }
    if (buttons & 0x08) {
      ((VentControlSettings *)getSettings(g_activePatient))->incrementIERatioIndex(-1);
      setDisplayOverride(k_modeIE);
    }
    if (buttons & 0x40) {
      ((VentControlSettings *)getSettings(g_activePatient))->incrementBPM(-1);
      setDisplayOverride(k_modeBPM);
    }
    if (buttons & 0x80) {
      ((VentControlSettings *)getSettings(g_activePatient))->incrementBPM(1);
      setDisplayOverride(k_modeBPM);
    }
  }
  // Hold both buttons for 15 seconds to reset all relays
  if (buttons == 0x30) {
    if (g_holdingForReset) {
      if (g_lastAnyKey > g_holdReset + 15000UL) {
        Serial.println("Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! Reset! ");
        for (int i = 0; i < 8; i++) {
          g_relay[i].resetCycles();
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
      unsigned long diff = g_lastAnyKey - g_holdOnOff;
      char buffer[9];
      snprintf(buffer, 9, "ON-OFF %d", (int) (6000 - diff)/1000);
      setDisplayString(buffer, 1000UL);
      if (g_lastAnyKey > g_holdOnOff + 5000UL) {
        Serial.println("Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! Toggle Relay! ");
        ((VentControlSettings *)getSettings(g_activePatient))->setEnabled(!getEnabled(g_activePatient));
        setDisplayString("SWITCHED", 2000UL);
        g_holdingForOnOff = false;
      }
    } else {
      // Released the button early
      g_holdingForOnOff = false;
      g_lastKey = 0;
      setDisplayOverride(k_modeDefault);
    }
  } else if (buttons == 0x20) {
    g_holdingForOnOff = true;
    g_holdOnOff = g_lastAnyKey;
  }
  if (g_holdingForRelayCount) {
    if (buttons == 0x10) {
      if (g_lastAnyKey > g_holdRelayCount + 700UL) {
        char buffer[9];
        unsigned long cycles = g_relay[g_activePatient].getCycles();
        if (cycles < 1000000) {
          snprintf(buffer, 9, "%d %6ld", g_activePatient + 1, cycles);
        } else {
          snprintf(buffer, 9, "%d%4ld E3", g_activePatient + 1, cycles/1000);
        }
        setDisplayString(buffer, 1000UL);
      }
      if (g_lastAnyKey > g_holdRelayCount + 10000UL) {
        g_relay[g_activePatient].resetCycles();
        setDisplayString("RESET", 1000UL);
      }
    } else {
      // Released the button early
      g_holdingForRelayCount = false;
      g_lastKey = 0;
    }
  } else if (buttons == 0x10) {
    g_holdingForRelayCount = true;
    g_holdRelayCount = g_lastAnyKey;
    setDisplayString("CyCLES", 800UL);
  }
  checkSettings(g_activePatient);

  return (buttons != 0);
}
