/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _BOARD_B_BLE_H_
#define _BOARD_B_BLE_H_

#include "serial.h"
#include "nvs_conf.h"
#include "board_b.h"
#include <BLEDevice.h>
#include <BLE2902.h>
#include "BLEHIDDevice.h"

//BLE MANAGEMENT ROUTINES

extern BLEServer *pServer;
extern BLECharacteristic * pTxCharacteristic;
extern BLEService *pService;


void initBleService();
uint16_t getBLEMTU();

#endif
