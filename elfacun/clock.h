/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

//CLOCK DATA AND ROUTINES

#include "serial.h"
#include "board_a.h"

extern int lmillis ;
extern byte lhours ;
extern byte lmins ;
extern byte lsecs ;
extern boolean lCountsDown ;
extern boolean lRunning ;
extern int rmillis ;
extern byte rhours ;
extern byte rmins ;
extern byte rsecs ;
extern boolean rCountsDown ;
extern boolean rRunning ;
extern boolean leverPositionHighRight ;
extern boolean rightTurn ;

extern boolean lTimeSymbol ;
extern boolean lFischSymbol ;
extern boolean lDelaySymbol ;
extern boolean lHglassSymbol ;
extern boolean lUpcntSymbol ;
extern boolean lByoSymbol ;
extern boolean lEndSymbol ;
extern boolean lPeriod ;
extern boolean lFlagSymbol ;
extern boolean lSoundSymbol ;
extern boolean lBlackSymbol ;

extern boolean rTimeSymbol ;
extern boolean rFischSymbol ;
extern boolean rDelaySymbol ;
extern boolean rHglassSymbol ;
extern boolean rUpcntSymbol ;
extern boolean rByoSymbol ;
extern boolean rEndSymbol ;
extern boolean rPeriod ;
extern boolean rFlagSymbol ;
extern boolean rSoundSymbol ;
extern boolean rWhiteSymbol ;

extern boolean keepIcons ;
extern boolean batSymbol ;

extern boolean displayingMessage ;

extern boolean isExternalClock ;
extern boolean refreshClock ;



//CLOCK TIMER
extern hw_timer_t * timer;
extern volatile SemaphoreHandle_t timerSemaphore;
extern portMUX_TYPE timerMux ;


void IRAM_ATTR onTimer();


#endif
