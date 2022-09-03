/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _SERIAL_H_
#define _SERIAL_H_

//SERIAL MANAGEMENT ROUTINES

#include "Arduino.h"
#include "BluetoothSerial.h"

//If any debug is enabled the USB port is disabled for board communication
//So the best setup for debugging is to have the board connected via bluetooth with the host and debug via serial USB
//serialDebug- verbosely dump all communications over the serial port
const boolean serialDebug = false;
//serialLog- dump debug information over the serial port
const boolean serialLog = false;
//serialMirror- mirror BT comms over USB serial
const boolean serialMirror = false;

extern BluetoothSerial SerialBT;
extern boolean btConnected;

byte readSerial();
byte readSerialBlock();
void readSerial(byte* buffer, int len);
void writeSerial(byte data);
void writeSerial(byte* data, int len);
void printSerial(String data);
void printSerial(String data, int len);
void commitSerial();

#endif
