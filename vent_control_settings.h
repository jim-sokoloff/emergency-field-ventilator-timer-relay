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

#ifndef VENT_CONTROL_SETTINGS_H
#define VENT_CONTROL_SETTINGS_H

#include <Arduino.h>

extern long g_serialSpeed;

class IERatio {
  public:
    IERatio(byte i = 10, byte e = 20, int inhale_percentage_in_bpp = 3333);
    byte i;
    byte e;
    int inhale_percentage_in_bpp;
};


class VentControlSettings {
  public:
    VentControlSettings(void);

    void checkSettings(void);
    void selectIndex(byte index);
    void incrementIERatioIndex(int offset = 1);
    void incrementBPM(int offset = 1);
    void setBPM10ths(int breathInTenthsPerMinute);
    void setEnabled(byte enabled);

    unsigned long getTotalDurationInMS(void);
    unsigned long getInhaleDurationInMS(void);
    unsigned long getExhaleDurationInMS(void);
  public:  // HACK
    int breathInTenthsPerMinute;
    IERatio ieRatio;
    byte ieRatioIndex;
    byte enabled;
};


const VentControlSettings *getSettings(int patient);
void checkSettings(int patient);
byte getEnabled(int patient);

#endif // VENT_CONTROL_SETTINGS_H
