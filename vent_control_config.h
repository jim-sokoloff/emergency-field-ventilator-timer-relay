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

#ifndef VENT_CONTROL_CONFIG_H
#define VENT_CONTROL_CONFIG_H

extern const char *g_LEDVersion;
extern const char *g_version;

// If you're trying to squeeze the code onto a smaller micro,
// set this to 1 and it will minimize the amount of text output
#define LIMITED_MEMORY 0

// Developers should use this:
#define DEBUG 1


#endif // # VENT_CONTROL_CONFIG_H
