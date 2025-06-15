/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _SYSTEM_UPDATE_H_
#define _SYSTEM_UPDATE_H_

#include <SD.h>
#include <FS.h>
#include <Update.h>
#include "screen.h"
#include "crc32.h"

void updateFromFS(fs::FS &fs);
void checkRemoteUpdateFromFS(fs::FS &fs);
void performRemoteUpdate();
void load_file_from_sd_to_spiffs(String filename);

extern uint32_t remoteUpdateFileSize;
extern char* remoteUpdateFileCRC;
extern boolean sendRemoteUpdateFileOverBLE;
extern char* remoteUpdateFileName;



#endif
