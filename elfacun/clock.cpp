/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "clock.h"

int lmillis = 0;
byte lhours = 0;
byte lmins = 0;
byte lsecs = 0;
boolean lCountsDown = true;
boolean lRunning = false;
int rmillis = 0;
byte rhours = 0;
byte rmins = 0;
byte rsecs = 0;
boolean rCountsDown = true;
boolean rRunning = false;
boolean leverPositionHighRight = true;
boolean rightTurn = true;

boolean lTimeSymbol = false;
boolean lFischSymbol = false;
boolean lDelaySymbol = false;
boolean lHglassSymbol = false;
boolean lUpcntSymbol = false;
boolean lByoSymbol = false;
boolean lEndSymbol = false;
boolean lPeriod = 0;
boolean lFlagSymbol = false;
boolean lSoundSymbol = false;
boolean lBlackSymbol = false;

boolean rTimeSymbol = false;
boolean rFischSymbol = false;
boolean rDelaySymbol = false;
boolean rHglassSymbol = false;
boolean rUpcntSymbol = false;
boolean rByoSymbol = false;
boolean rEndSymbol = false;
boolean rPeriod = 0;
boolean rFlagSymbol = false;
boolean rSoundSymbol = false;
boolean rWhiteSymbol = false;

boolean keepIcons = false;
boolean batSymbol = false;

boolean displayingMessage = false;

boolean isExternalClock = false;
boolean refreshClock = false;



//CLOCK TIMER
//It fires every 50ms to update emulated chess clock

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  if (rRunning) {
    if (rCountsDown) {
      rmillis = max(0, rmillis - 50 );
    } else {
      rmillis += 50;
    }

    byte lastsecs = rsecs;

    rhours = rmillis / 3600000;
    rmins = (rmillis - (rhours * 3600000)) / 60000;
    rsecs = (rmillis - (rhours * 3600000) - (rmins * 60000)) / 1000;
    if (lastsecs != rsecs) {
      refreshClock = true;
    }

  }

  if (lRunning) {
    if (lCountsDown ) {
      lmillis = max(0, lmillis - 50 );
    } else {
      lmillis += 50;
    }

    byte lastsecs = lsecs;

    lhours = lmillis / 3600000;
    lmins = (lmillis - (lhours * 3600000)) / 60000;
    lsecs = (lmillis - (lhours * 3600000) - (lmins * 60000)) / 1000;
    if (lastsecs != lsecs) {
      refreshClock = true;
    }

  }

  if (updateMode == UPDATE_NORMAL) {
    send_clock();
  }

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  //xSemaphoreGiveFromISR(timerSemaphore, NULL);
}
