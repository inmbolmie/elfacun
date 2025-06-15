/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _BOARD_C_BLE_H_
#define _BOARD_C_BLE_H_


#include "serial.h"
#include "screen.h"
#include "nvs_conf.h"
#include "board_b.h"
#include "board_c.h"
#include <BLEDevice.h>
#include <BLE2902.h>
#include "BLEHIDDevice.h"

//BLE MANAGEMENT ROUTINES

extern uint8_t modeCmac[8];

extern BLEServer *pServerC;
extern BLECharacteristic * pTxCharacteristicC;
extern BLECharacteristic * pTxCharacteristicC2;
extern BLECharacteristic * pTxCharacteristicC3;
extern BLEService *pServiceC;
extern boolean BLECdeviceConnected;

void initBleC();
void initBleServiceC();
uint16_t getBLEMTUC();

void startAdvertisement();
void stopAdvertisement();
void sendPositionBLEBoardC();

#endif
