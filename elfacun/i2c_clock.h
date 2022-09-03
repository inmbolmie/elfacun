/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _EFI2C_H_
#define _EFI2C_H_

//I2C CLOCK MANAGEMENT ROUTINES

#include <Arduino.h>
#include <driver/i2c.h>
#include "util.h"
#include "board.h"

#define INCLUDE_vTaskSuspend                    1

const byte BUTTON_BACK = 0x31;
const byte BUTTON_PLUS = 0x32;
const byte BUTTON_PLAY = 0x33;
const byte BUTTON_MINUS = 0x34;
const byte BUTTON_NEXT = 0x35;
const byte BUTTON_LEVER = 0x40; //???



boolean isPhysicalClockReversed();
void configure_slave_addr0();
void configure_slave_addr10();
void configure_master_addr10();
int recvi2c();  //intenal
byte writei2c(int addr, uint8_t* data, int size, boolean waitAck, boolean forceWait);  //internal
boolean initializei2c();
void runExternalClockLoop() ;
void externalClockEndDisplay();
void externalClockEndDisplayTask();
void externalClockSetDisplay(String message);
void externalClockSetDisplayTask();
void externalClockSetAndRun(int lhours, int lmins, int lsecs, boolean lCountsDown, boolean lRunning, int rhours, int rmins, int rsecs,  boolean rCountsDown, boolean rRunning);
void externalClockSetAndRunTask();
void externalClockCheckMessages();

void checkForPendingExternalClockMessages();

void processAckReceivedTask(byte* commandDataBytes);
void processButtonChangeReceivedTask(byte* commandDataBytes);
void processTimeReceivedTask(byte* commandDataBytes);

//void processPendingExternalClockMessageAck();
void processPendingExternalClockMessageButtonChange(int button);
void processPendingExternalClockMessageTime(int lhours,int  lmins,int  lsecs,int  rhours,int  rmins,int  rsecs,int  leverState,int  updt);



#endif
