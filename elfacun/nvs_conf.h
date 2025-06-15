/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/


#ifndef _NVS_CONF_H_
#define _NVS_CONF_H_

//Default configuration

#include <SD.h>
#include <FS.h>
#include <SDConfig.h>
#include "screen.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lichess.h"

extern char* baseMacAddress ;
extern char* modeABtAdvertisedName ;
extern char* modeBBtAdvertisedName ;
extern char* modeCBtAdvertisedName ;
extern char* defaultMode ;
extern char* buzzerEnabled ;
extern char* buzzerEnabledPassiveLed ;
extern char* ledsEnabled ;
extern char* dualLedsEnabled ;

extern char* lichessAutoInvertEnabled;

extern char* revelationButtonOrder;

extern char* token;
extern char* ssid;
extern char* password;

extern char* lichesst1;
extern char* lichessi1;
extern char* lichessr1;
extern char* lichessc1;
extern char* lichesse1;

extern char* lichesst2;
extern char* lichessi2;
extern char* lichessr2;
extern char* lichessc2;
extern char* lichesse2;

extern char* lichesst3;
extern char* lichessi3;
extern char* lichessr3;
extern char* lichessc3;
extern char* lichesse3;

extern char* lichesst4;
extern char* lichessi4;
extern char* lichessr4;
extern char* lichessc4;
extern char* lichesse4;

extern char* lichesst5;
extern char* lichessi5;
extern char* lichessr5;
extern char* lichessc5;
extern char* lichesse5;

extern char* lichesst6;
extern char* lichessi6;
extern char* lichessr6;
extern char* lichessc6;
extern char* lichesse6;

extern char* screenBrightnessDefault ;

extern char* btPower ;

extern int freqTone1;
extern int freqTone2;
extern int freqTone3;
extern int durationTone1;
extern int durationTone2;
extern int durationTone3;

extern char* scallyHomage;


void initialize_nvs();
void read_config_nvs();
void read_config_sd_store_nvs() ;
void storeNvsValue(char* parameter, char * value);
void storeNVSBoardPosition();
void deleteNVSBoardPosition();
boolean recoverNVSBoardPosition();
void setLichessGameInProgress();
void setLichessGameFinished();
boolean isLichessGameInProgress();

extern boolean enableNvsLog;


#endif
