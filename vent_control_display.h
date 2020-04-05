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

#ifndef VENT_CONTROL_DISPLAY_H
#define VENT_CONTROL_DISPLAY_H

#include <Arduino.h>

enum DisplayModes { k_modeDefault = 0, k_modeMessage, k_modeBPM, k_modeIE, k_modeKeyCountdown };

byte displayGetButtons();

void setDisplayOverride(int mode, unsigned long duration = 5000UL);

void setDisplayString(const char *string, unsigned long duration);

void updateDisplay(unsigned long now);


#endif // VENT_CONTROL_DISPLAY_H
