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

#ifndef VENT_CONTROL_EEPROM_H
#define VENT_CONTROL_EEPROM_H

bool readEEPROM();
void updateEEPROM(unsigned long now, bool force = false);
void scheduleEEPROMUpdate(unsigned long delay = 5000UL);

#endif // VENT_CONTROL_EEPROM_H
