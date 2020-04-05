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
#include "vent_control_display.h"
#include "vent_control_globals.h"
#include "vent_control_relay.h"
#include "vent_control_settings.h"
#include "vent_control_timer.h"

#include <Arduino.h>
#include <TM1638.h>

#if LIMITED_MEMORY
const char *g_messages[] = { " TImER ", "PROUIDED", " AS-IS", "   No   ", "WARRAnty", " OF Any ", "TYPE.   ", "        ", "USER    ", "ASSUmES ", "ALL RISH", "        ", " SERIAL " };
#else
const char *g_messages[] = { " TImER ", "PROUIDED", " AS-IS", "   No   ", "WaRRanty", " OF Any ", "TYPE.   ", "        ", "USER    ", "ASSUmES ", "ALL RISH", "        ", "DEEP    ", "    DEEP", "REspEct ", "For ALL ", "MEDICAL ", "StaFF!  ", "ThankYou", "For What", "You arE ", "doing!  ", "LOUE USA", "        ", " SERIAL " };
#endif
const int g_numStartupMessages = sizeof(g_messages)/sizeof(g_messages[0]);
const unsigned long g_messageTime = 1200UL;

const int tm1638_data = 11;
const int tm1638_clock = 12;
const int tm1638_strobe = 13;

TM1638 module1(tm1638_data, tm1638_clock, tm1638_strobe);

int g_modeOverride = k_modeDefault;
unsigned long g_modeOverrideTimer = 0;
char g_displayMessage[9];


void setDisplayString(const char *string, unsigned long duration) {
  for (int i = 0; i < 8; i++)
    g_displayMessage[i] = ' ';

  int i = 0;
  for (char *s = (char *)string, *d = g_displayMessage; (*d++ = *s++) && i < 8; i++ ) {
    ;
  }
  g_displayMessage[8] = 0;
  setDisplayOverride(k_modeMessage, duration);
}

byte displayGetButtons() {
  return module1.getButtons();
}
void setDisplayOverride(int mode, unsigned long duration) {
  g_modeOverride = mode;
  g_modeOverrideTimer = millis() + duration;
}

bool doInitialDisplay(unsigned long now) {
  int message = now/g_messageTime;

  if (message > g_numStartupMessages + 3)
    return false;
    
  if (message < g_numStartupMessages)
    setDisplayString(g_messages[message], 1500UL);
  else if (message == g_numStartupMessages) {
    char buffer[9];
    sprintf(buffer, "%ld", g_serialSpeed);
    setDisplayString(buffer, 1500UL);
  } else {
    setDisplayString(g_LEDVersion, 1500UL);
  }

  return true;
}


int getDisplayMode(unsigned long now) {
  int mode = ((now % 5000UL) < 2500) ? k_modeBPM : k_modeIE;           // Default calc is 50:50 BPM and IERatio with a 5 second period.
  if (g_modeOverride) {
    if (now > g_modeOverrideTimer) {
      g_modeOverride = k_modeDefault;
    } else {
      mode = g_modeOverride;
    }
  }
  return mode;
}

void displayOnOffLEDs() {
  for (int i = 0; i < 8; i++) {
    module1.setLED(g_relay[i].exhaling, i);
  }
}

void displayPatient(int patientIndex) {
  module1.setDisplayDigit(patientIndex + 1, 0, 0);
}

void displayDS (int ds, int digit) {
  module1.setDisplayDigit(ds/10, digit, 1);
  module1.setDisplayDigit(ds%10, digit+1, 0); 
}

void displayStringAt(const char *s, const byte pos, const byte font[] = FONT_DEFAULT) {
  int stringLength = strlen(s);

  for (int i = 0; i < 8 - pos; i++) {
    if (i < stringLength) {
      module1.sendChar(i + pos, font[s[i] - 32], 0);
    } else {
      break;
    }
  }
}

void displayPatientIERatio(unsigned long now, int patientIndex) {
  (void) now;
  
  const VentControlSettings *settings;
  
  settings = getSettings(patientIndex);
  byte i = settings->ieRatio.i;
  byte e = settings->ieRatio.e;
  if (i == 10) {
    module1.setDisplayDigit(1, 2, false);
    if (e == 10) {
      module1.setDisplayDigit(1, 6, false);
    } else {
      displayDS(e, 6);
    }
    module1.sendChar(4, 0x09, false);   // "colon"
  } else {
    displayDS(i, 2);
    module1.setDisplayDigit(1, 6, false);
    module1.sendChar(5, 0x09, false);   // "colon"
  }
  
}

void displayPatientBPM(unsigned long now, int patientIndex) {
  (void) now;
    
  int bp10m = getSettings(patientIndex)->breathInTenthsPerMinute;
  displayStringAt("bPM", 2);
  module1.setDisplayDigit(bp10m/100, 5, false);
  displayDS(bp10m%100, 6);
}


void displayPatientSettings(unsigned long now, int patientIndex) {
  module1.clearDisplay();
  int mode = getDisplayMode(now);
  int toggleMessage = (now % 2500) < 1000;
   
  switch (mode) {
    case k_modeMessage:
      module1.setDisplayToString(g_displayMessage);
      break;
    case k_modeBPM:
      if (getEnabled(patientIndex) || toggleMessage) {
        displayPatientBPM(now, patientIndex);
      } else {
        module1.setDisplayToString(F("  OFF b6"));
      }
      displayPatient(patientIndex);
      break;
    case k_modeIE:
      if (getEnabled(patientIndex) || toggleMessage) {    
        displayPatientIERatio(now, patientIndex);
      } else {
        module1.setDisplayToString(F("  OFF b6"));
      }
      displayPatient(patientIndex);
      break;
    default:
      break;
  }
}
void displayBreathLEDs(unsigned long now, int patientIndex) {
  unsigned long ticksLeft = g_relay[patientIndex].nextToggle - now;
  unsigned long target = g_relay[patientIndex].duration;
  int leds = (target - ticksLeft) * 8 / target;
  bool inhale = !g_relay[g_activePatient].exhaling;

  if (!inhale) {
    leds = 8 - leds; 
  }
  for (int i = 0; i < 8; i++) {
    module1.setLED(i < leds, i);
  }
}

void displayOnOffLEDs(unsigned long now) {
  (void) now;
  
  for (int i = 0; i < 8; i++) {
    module1.setLED(g_relay[i].exhaling, i);
  };
}

void updateLEDs(unsigned long now, int patientIndex) {
  int mode = getDisplayMode(now);
  if (!getEnabled(patientIndex)) {
    displayOnOffLEDs(now);
  } else {
    switch (mode) {
      case k_modeBPM:
      case k_modeIE:
        displayBreathLEDs(now, patientIndex);
        break;
      default:
        displayOnOffLEDs(now);
      break;
    }
  }
}
void updateDisplay(unsigned long now) {
  // Required to allow us to hot-plug the TM1638 module
  module1.setupDisplay(true, 7);

  doInitialDisplay(now);
  
  displayPatientSettings(now, g_activePatient);
  updateLEDs(now, g_activePatient);
}
