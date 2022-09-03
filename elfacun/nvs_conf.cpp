/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "nvs_conf.h"


char* baseMacAddress = "";
char* modeABtAdvertisedName = "ELFACUN CHESS";
char* modeBBtAdvertisedName = "ELFACUN CHESS";
char* defaultMode = "A";
char* buzzerEnabled = "TRUE";
char* ledsEnabled = "TRUE";


char* lichessAutoInvertEnabled = "TRUE";

char* revelationButtonOrder = "FALSE";

char* token    = "";
char* ssid     = "";
char* password = "";

char* lichesst1 = "";
char* lichessi1 = "";
char* lichessr1 = "";
char* lichessc1 = "";

char* lichesst2 = "";
char* lichessi2 = "";
char* lichessr2 = "";
char* lichessc2 = "";

char* lichesst3 = "";
char* lichessi3 = "";
char* lichessr3 = "";
char* lichessc3 = "";

char* lichesst4 = "";
char* lichessi4 = "";
char* lichessr4 = "";
char* lichessc4 = "";

char* lichesst5 = "";
char* lichessi5 = "";
char* lichessr5 = "";
char* lichessc5 = "";

char* lichesst6 = "";
char* lichessi6 = "";
char* lichessr6 = "";
char* lichessc6 = "";



void initialize_nvs() {
  // Initialize NVS to store/read configuration
  tft.println("Initializing NVS");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {

    tft.println("Error initializing NVS");
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
    delay(10000);
  }
  ESP_ERROR_CHECK( err );
}


boolean readNvsValue(nvs_handle my_handle, char** parameter, char * nvsKey, boolean convertToUpcase) {

  esp_err_t err;
  char* readValue;
  size_t readValueLen;

  err = nvs_get_str(my_handle, nvsKey, NULL, &readValueLen);
  if (err == ESP_OK) {
    readValue = new char[readValueLen];
    err = nvs_get_str(my_handle, nvsKey, readValue, &readValueLen);
    if (err == ESP_OK) {

      if (convertToUpcase) {
        upcaseCharArray(readValue, readValueLen);
      }

      *parameter = readValue;
      
      tft.print("Read from NVS: ");
      tft.println(nvsKey);
      //tft.println(*parameter);
    } else {
      tft.printf("Error (%s) reading value from NVS\n", esp_err_to_name(err));
      delay(10000);
      return false;
    }
  } else if (err == ESP_ERR_NVS_NOT_FOUND) {
    tft.printf("Key not found in NVS :");
    tft.printf(nvsKey);
    tft.printf("\n");
    return false;
  } else {
    tft.printf("Error (%s) reading value length from NVS\n", esp_err_to_name(err));
    return false;
    //delay(10000);
  }

  return true;
}




void read_config_nvs() {
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    tft.printf("Error (%s) opening NVS for reading\n", esp_err_to_name(err));
    delay(10000);
  } else {
    //Read config
    tft.println("Reading config from NVS");

    readNvsValue(my_handle, &baseMacAddress, "basemac", true);
    readNvsValue(my_handle, &modeABtAdvertisedName, "modeabtname", true);
    readNvsValue(my_handle, &modeBBtAdvertisedName, "modebbtname", true);
    readNvsValue(my_handle, &defaultMode, "modedefault", true);
    readNvsValue(my_handle, &buzzerEnabled, "buzzerenabled", true);
    readNvsValue(my_handle, &ledsEnabled, "ledsenabled", true);
    readNvsValue(my_handle, &tmString, "modeatm", false);

    readNvsValue(my_handle, &lichessAutoInvertEnabled, "lichessaie", true);

    readNvsValue(my_handle, &revelationButtonOrder, "revbutton", true);

    readNvsValue(my_handle, &ssid, "lichessssid", false);
    readNvsValue(my_handle, &password, "lichesspass", false);
    readNvsValue(my_handle, &token, "lichesstok", false);

    readNvsValue(my_handle, &lichesst1, "lichesst1", false);
    readNvsValue(my_handle, &lichessi1, "lichessi1", false);
    readNvsValue(my_handle, &lichessr1, "lichessr1", false);
    readNvsValue(my_handle, &lichessc1, "lichessc1", false);

    readNvsValue(my_handle, &lichesst2, "lichesst2", false);
    readNvsValue(my_handle, &lichessi2, "lichessi2", false);
    readNvsValue(my_handle, &lichessr2, "lichessr2", false);
    readNvsValue(my_handle, &lichessc2, "lichessc2", false);

    readNvsValue(my_handle, &lichesst3, "lichesst3", false);
    readNvsValue(my_handle, &lichessi3, "lichessi3", false);
    readNvsValue(my_handle, &lichessr3, "lichessr3", false);
    readNvsValue(my_handle, &lichessc3, "lichessc3", false);

    readNvsValue(my_handle, &lichesst4, "lichesst4", false);
    readNvsValue(my_handle, &lichessi4, "lichessi4", false);
    readNvsValue(my_handle, &lichessr4, "lichessr4", false);
    readNvsValue(my_handle, &lichessc4, "lichessc4", false);

    readNvsValue(my_handle, &lichesst5, "lichesst5", false);
    readNvsValue(my_handle, &lichessi5, "lichessi5", false);
    readNvsValue(my_handle, &lichessr5, "lichessr5", false);
    readNvsValue(my_handle, &lichessc5, "lichessc5", false);

    readNvsValue(my_handle, &lichesst6, "lichesst6", false);
    readNvsValue(my_handle, &lichessi6, "lichessi6", false);
    readNvsValue(my_handle, &lichessr6, "lichessr6", false);
    readNvsValue(my_handle, &lichessc6, "lichessc6", false);


    char* vermajorString;
    char* verminorString;
    char* verclockmajorString;
    char* verclockminorString;

    if (readNvsValue(my_handle, &vermajorString, "modeavermaj", false)) {
      vermajor = (uint8_t)atoi(vermajorString);
    }
    if (readNvsValue(my_handle, &verminorString, "modeavermin", false)) {
      verminor = (uint8_t)atoi(verminorString);
    }
    if (readNvsValue(my_handle, &verclockmajorString, "modeaverclmaj", false)) {
      verclockmajor = (uint8_t)atoi(verclockmajorString);
    }
    if (readNvsValue(my_handle, &verclockminorString, "modeaverclmin", false)) {
      verclockminor = (uint8_t)atoi(verclockminorString);
    }

    readNvsValue(my_handle, &serialNumber, "modeaserial", false);
    readNvsValue(my_handle, &shortSerialNumber, "modeasserial", false);

    nvs_close(my_handle);
  }
}



void saveNvsValue(nvs_handle my_handle, char* parameter, char * value) {

  esp_err_t err = nvs_set_str(my_handle , parameter, value);
  if (err != ESP_OK) {
    tft.printf("Error (%s) writing value in NVS\n", esp_err_to_name(err));
    delay(10000);
  }

}




void eraseAllNvsValues(nvs_handle my_handle) {

  esp_err_t err = nvs_erase_all(my_handle);
  if (err != ESP_OK) {
    tft.printf("Error (%s) erasing all values in NVS\n", esp_err_to_name(err));
    delay(10000);
  }

}


void read_config_sd_store_nvs() {
  //Read config from SD and store in the NVS

  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    tft.printf("Error (%s) opening NVS for writing\n", esp_err_to_name(err));
    delay(10000);
  }

  SDConfig cfg;
  if (cfg.begin("/settings.txt", 1000)) {

    tft.println("settings.txt found");

    eraseAllNvsValues(my_handle);
    
    while (cfg.readNextSetting()) {

      tft.print("Reading setting: ");
      tft.println(cfg.getName());
      tft.print("Value: ");
      tft.println(cfg.copyValue());

      if (cfg.nameIs("base.mac.address")) {
        saveNvsValue(my_handle, "basemac", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.bt.advertised.name")) {
        saveNvsValue(my_handle, "modeabtname", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.trademark")) {
        saveNvsValue(my_handle, "modeatm", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.version.major")) {
        saveNvsValue(my_handle, "modeavermaj", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.version.minor")) {
        saveNvsValue(my_handle, "modeavermin", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.clock.version.major")) {
        saveNvsValue(my_handle, "modeaverclmaj", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.clock.version.minor")) {
        saveNvsValue(my_handle, "modeaverclmin", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.serial.number.long")) {
        saveNvsValue(my_handle, "modeaserial", cfg.copyValue());

      } else if (cfg.nameIs("mode.a.serial.number.short")) {
        saveNvsValue(my_handle, "modeasserial", cfg.copyValue());

      } else if (cfg.nameIs("mode.b.bt.advertised.name")) {
        saveNvsValue(my_handle, "modebbtname", cfg.copyValue());

      } else if (cfg.nameIs("mode.default")) {
        saveNvsValue(my_handle, "modedefault", cfg.copyValue());

      } else if (cfg.nameIs("buzzer.enabled")) {
        saveNvsValue(my_handle, "buzzerenabled", cfg.copyValue());
        
      } else if (cfg.nameIs("leds.enabled")) {
        saveNvsValue(my_handle, "ledsenabled", cfg.copyValue());
      } 
      
        else if (cfg.nameIs("lichess.board.autoinvert")) {
        saveNvsValue(my_handle, "lichessaie", cfg.copyValue());
      } 
        else if (cfg.nameIs("buttons.order.revelation")) {
        saveNvsValue(my_handle, "revbutton", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.wifi.ssid")) {
        saveNvsValue(my_handle, "lichessssid", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.wifi.password")) {
        saveNvsValue(my_handle, "lichesspass", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.wifi.authtoken")) {
        saveNvsValue(my_handle, "lichesstok", cfg.copyValue());
      }


      else if (cfg.nameIs("lichess.mode.1.time")) {
        saveNvsValue(my_handle, "lichesst1", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.1.increment")) {
        saveNvsValue(my_handle, "lichessi1", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.1.rated")) {
        saveNvsValue(my_handle, "lichessr1", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.1.color")) {
        saveNvsValue(my_handle, "lichessc1", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.mode.2.time")) {
        saveNvsValue(my_handle, "lichesst2", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.2.increment")) {
        saveNvsValue(my_handle, "lichessi2", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.2.rated")) {
        saveNvsValue(my_handle, "lichessr2", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.2.color")) {
        saveNvsValue(my_handle, "lichessc2", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.mode.3.time")) {
        saveNvsValue(my_handle, "lichesst3", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.3.increment")) {
        saveNvsValue(my_handle, "lichessi3", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.3.rated")) {
        saveNvsValue(my_handle, "lichessr3", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.3.color")) {
        saveNvsValue(my_handle, "lichessc3", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.mode.4.time")) {
        saveNvsValue(my_handle, "lichesst4", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.4.increment")) {
        saveNvsValue(my_handle, "lichessi4", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.4.rated")) {
        saveNvsValue(my_handle, "lichessr4", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.4.color")) {
        saveNvsValue(my_handle, "lichessc4", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.mode.5.time")) {
        saveNvsValue(my_handle, "lichesst5", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.5.increment")) {
        saveNvsValue(my_handle, "lichessi5", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.5.rated")) {
        saveNvsValue(my_handle, "lichessr5", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.5.color")) {
        saveNvsValue(my_handle, "lichessc5", cfg.copyValue());
      }

      else if (cfg.nameIs("lichess.mode.6.time")) {
        saveNvsValue(my_handle, "lichesst6", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.6.increment")) {
        saveNvsValue(my_handle, "lichessi6", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.6.rated")) {
        saveNvsValue(my_handle, "lichessr6", cfg.copyValue());
      } else if (cfg.nameIs("lichess.mode.6.color")) {
        saveNvsValue(my_handle, "lichessc6", cfg.copyValue());


      } else {
        tft.print("Unknown setting: ");
        tft.println(cfg.getName());
      }
    }
    cfg.end();

    tft.println("Committing settings in NVS ... ");
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
      tft.println("Error committing NVS");
      delay(10000);
    } else {
      tft.println("Commit settings in NVS OK");
      delay(2000);
    }

    nvs_close(my_handle);

  } else {
    tft.println("settings.txt not found");
    delay(2000);
  }

}
