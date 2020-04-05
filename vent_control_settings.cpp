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

#include "vent_control_settings.h"
#include "vent_control_EEPROM.h"

const IERatio g_IESteps[] = { {10, 50, 1667}, {10, 45, 1806}, {10, 41, 1944}, {10, 38, 2083}, {10, 35, 2222}, 
                              {10, 32, 2361}, {10, 30, 2500}, {10, 28, 2639}, {10, 26, 2778}, {10, 24, 2917},
                              {10, 23, 3056}, {10, 21, 3194}, {10, 20, 3333}, {10, 19, 3472}, {10, 18, 3611},
                              {10, 17, 3750}, {10, 16, 3889}, {10, 15, 4028}, {10, 14, 4167}, {10, 13, 4306},
                              {10, 13, 4444}, {10, 12, 4583}, {10, 11, 4722}, {10, 11, 4861}, {10, 10, 5000},
                              {11, 10, 5139}, {11, 10, 5278}, {12, 10, 5417}, {13, 10, 5556}, {13, 10, 5694},
                              {14, 10, 5833}, {15, 10, 5972}, {16, 10, 6111}, {17, 10, 6250}, {18, 10, 6389},
                              {19, 10, 6528}, {20, 10, 6667}, {21, 10, 6806}, {23, 10, 6944}, {24, 10, 7083},
                              {26, 10, 7222}, {28, 10, 7361}, {30, 10, 7500}, {32, 10, 7639}, {35, 10, 7778},
                              {38, 10, 7917}, {41, 10, 8056}, {45, 10, 8194}, {50, 10, 8333} };

const byte k_numIESteps = sizeof(g_IESteps)/sizeof(g_IESteps[0]); 

long g_serialSpeed = 115200;

IERatio::IERatio(byte i, byte e, int inhale_percentage_in_bpp) : i(i), e(e), inhale_percentage_in_bpp(inhale_percentage_in_bpp) {
}

VentControlSettings::VentControlSettings() {
  breathInTenthsPerMinute = 200;
  ieRatioIndex = k_numIESteps / 4;
  enabled = 1;
  this->checkSettings();
}

void VentControlSettings::selectIndex(byte index) {
  ieRatioIndex = index;
  ieRatio = g_IESteps[index];
}

void VentControlSettings::setBPM10ths(int breathInTenthsPerMinute) {
  this->breathInTenthsPerMinute = breathInTenthsPerMinute;
}

void VentControlSettings::setEnabled(byte enabled) {
  this->enabled = enabled;
}
    
void VentControlSettings::checkSettings() {
  ieRatioIndex = constrain(ieRatioIndex, 0, k_numIESteps-1);
  selectIndex(ieRatioIndex);
  
  breathInTenthsPerMinute = breathInTenthsPerMinute / 5 * 5;
  breathInTenthsPerMinute = constrain(breathInTenthsPerMinute, 100, 300);
}

VentControlSettings g_settings[8];

void checkSettings(int patient) {
  g_settings[patient].checkSettings();  
}

const VentControlSettings *getSettings(int patient) {
  return &g_settings[patient];
}

void VentControlSettings::incrementIERatioIndex(int offset) {
  selectIndex(ieRatioIndex + offset);
  scheduleEEPROMUpdate( 5UL * 1000UL);
}
void VentControlSettings::incrementBPM(int offset) {
  breathInTenthsPerMinute += offset * 5;
  scheduleEEPROMUpdate( 5UL * 1000UL);
}

byte getEnabled(int patient) {
  return g_settings[patient].enabled;
}

unsigned long VentControlSettings::getTotalDurationInMS(void) {
  unsigned long duration = 600000 / breathInTenthsPerMinute;
  return duration;
}
unsigned long VentControlSettings::getInhaleDurationInMS(void) {
  unsigned long duration = getTotalDurationInMS();
  duration = duration * (        ieRatio.inhale_percentage_in_bpp) / 10000UL;
  
  return duration;
}
unsigned long VentControlSettings::getExhaleDurationInMS(void) {
  unsigned long duration = getTotalDurationInMS();
  duration = duration * (10000 - ieRatio.inhale_percentage_in_bpp) / 10000UL;
  
  return duration;
}
