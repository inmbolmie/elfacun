/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _RFID_H_
#define _RFID_H_

//SPI RFID MANAGEMENT ROUTINES

#include <MFRC522.h>
#include <SPI.h>
#include "serial.h"


void rfid_init();
boolean programRfidTag(int piece);
boolean readRfidTag(byte* liftedPiece);
boolean programRfidTag(int piece);

#endif
