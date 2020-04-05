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

#ifndef VENT_CONTROL_RELAY_H
#define VENT_CONTROL_RELAY_H


class RelayInfo {
  public:
    RelayInfo(byte index);

    unsigned long getCycles(void);
    void setCycles(unsigned long cycles);
    void resetCycles(void);
    unsigned long incrementCycles(unsigned long offset = 1UL);
    
    void updateRelay(unsigned long now);
    
    void setHighToExhale(bool highToExhale);
  public:  // HACK
    unsigned long nextToggle;
    bool exhaling;
    unsigned long duration;
  private:
    void setRelay(void);
    
    unsigned long cycles;
    //bool state;
    bool onToExhale;
    byte index;
    byte pin;
};

void updateRelays(unsigned long now);

extern RelayInfo g_relay[];

#endif // VENT_CONTROL_RELAY_H
