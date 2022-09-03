/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _BOARD_A_H_
#define _BOARD_A_H_

#include "util.h"
#include "i2c_clock.h"
#include "screen.h"
#include "lichess.h"

//Mode A board opcodes

#define MODE_A_RESET 0x40
#define MODE_A_CLOCK_REQUEST 0x41
#define MODE_A_CLOCK_COMMAND 0x2B
#define MODE_A_BOARD_REQUEST 0x42
#define MODE_A_UPDATE_REQUEST 0x43
#define MODE_A_UPDATE_BOARD_REQUEST 0x44
#define MODE_A_SERIAL_REQUEST 0x45
#define MODE_A_ADDRESS_REQUEST 0x46
#define MODE_A_DESCRIPTION_REQUEST 0x47
#define MODE_A_MOVES_REQUEST 0x49
#define MODE_A_UPDATE_LAZY 0x4B
#define MODE_A_BATTERY_LEVEL 0x4C
#define MODE_A_VERSION_REQUEST 0x4D
#define MODE_A_BOARD_REQUEST_BLACK 0x50
#define MODE_A_BOARD_SCAN_BLACK 0x51
#define MODE_A_BOARD_REQUEST_WHITE 0x52
#define MODE_A_BOARD_SCAN_WHITE 0x53
#define MODE_A_BOARD_SCAN_ALL 0x54
#define MODE_A_LONG_SERIAL_REQUEST 0x55
#define MODE_A_LEDS 0x60



//Magic data


//TM
extern char* tmString;

//Version
extern byte vermajor;
extern byte verminor;

//Clock version
extern byte verclockmajor;
extern byte verclockminor;


//Long Serial or Version
extern char* shortSerialNumber;
extern char* serialNumber ;


extern byte updateMode;
const byte UPDATE_NORMAL = 1;
const byte UPDATE_BOARD = 2;
const byte UPDATE_LAZY = 3;

//Buttons

extern boolean pushedBack;
extern boolean pushedMinus;
extern boolean pushedPlay;
extern boolean pushedPlus;
extern boolean pushedNext;
extern byte lastbuttons;

//LED TIMER
extern hw_timer_t * timerLed;
extern portMUX_TYPE timerLedMux;


void IRAM_ATTR onTimerLed();
void send_clock();
void set_clock(byte * content);
void board_a_clock_loop(byte command, byte * content, byte size ) ;
void board_a_command_loop(byte rcv);
void sendPendingUpdates();
void send_button_push(byte button);
void sendBoardDump();


#endif
