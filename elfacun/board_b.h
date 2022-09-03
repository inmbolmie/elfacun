/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _BOARD_B_H_
#define _BOARD_B_H_

#include "serial.h"
#include "board.h"
#include "ble_board_b.h"
#include "util.h"

//Mode B board opcodes
#define MB_S_STATUS   0x53
#define MB_L_LED      0x4C
#define MB_X_OFFLED   0x58
#define MB_T_RESET    0x54
#define MB_V_VERSION  0x56
#define MB_W_WRITE    0x57
#define MB_R_READ     0x52

//Mode B piece codes
#define BEMPTY       0x2E
#define BWPAWN       0x50
#define BWROOK       0x52
#define BWKNIGHT     0x4E
#define BWBISHOP     0x42
#define BWKING       0x4B
#define BWQUEEN      0x51
#define BBPAWN       0x70
#define BBROOK       0x72
#define BBKNIGHT     0x6E
#define BBBISHOP     0x62
#define BBKING       0x6B
#define BBQUEEN      0x71

void board_b_command_loop(byte rcv);
void sendStatusIfNeeded();
void ackMB_V_VERSION();
void ackMB_R_READ(byte rxValue1, byte rxValue2);
void ackMB_L_LED();
void ackMB_X_OFFLED();
void processLedBuffer() ;
void ackMB_W_WRITE(byte rxValue1, byte rxValue2, byte rxValue3, byte rxValue4) ;

#endif
