/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#include "Arduino.h"
#include "nvs_conf.h"


//Sound
extern boolean soundIsDisabled;
extern boolean soundIsDisabledPassiveLed;


inline byte decToBcd(byte val)
{
  return ( ((val / 10) << 4) | (val % 10) );
}


inline int bcdToDec(int value) {
  return (value / 16 * 10) + (value % 16);
}



void beep();
void boop();
void buup();


inline void reset() {
}


inline void upcaseCharArray(char* readValue, int readValueLen) {

  for (int i = 0; i < readValueLen - 1; i++) {
        readValue[i] = toupper(readValue[i]);
      }
}


#endif
