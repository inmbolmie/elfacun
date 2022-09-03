/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "system_update.h"



// perform system update
void performUpdate(Stream &updateSource, size_t updateSize, fs::FS &fs, char* updateFileName) {
  delay(1000);
  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      tft.println("Written : " + String(written) + " successfully");
    }
    else {
      tft.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end()) {
      tft.println("OTA done!");
      if (Update.isFinished()) {
        tft.println("Update successfully completed.");
        tft.println("Device will reboot in 15 seconds.");
        tft.println("update.bin has been removed from SD card.");
        tft.println("IF SD IS WRITE-PROTECTED REMOVE SD CARD NOW");
        fs.remove(updateFileName);
        delay(15000);
        tft.println("Rebooting...");
        delay(2000);
        ESP.restart();
      }
      else {
        tft.println("Update not finished. Something went wrong!");
        tft.println("Rebooting in 5 seconds");
        delay(5000);
        tft.println("Rebooting...");
        delay(2000);
        ESP.restart();
      }
    }
    else {
      tft.println("Error Occurred. Error #: " + String(Update.getError()));
      tft.println("Rebooting in 5 seconds");
      delay(5000);
      tft.println("Rebooting...");
      delay(2000);
      ESP.restart();
    }

  }
  else
  {
    tft.println("Not enough space to begin update");
    delay(5000);
  }
}




void updateFromFS(fs::FS &fs) {

  //  File root = fs.open("/");
  //
  //  File file = root.openNextFile();
  //  while (file) {
  //    if (file.isDirectory()) {
  //      tft.print("  DIR : ");
  //      tft.println(file.name());
  //
  //    } else {
  //      tft.print("  FILE: ");
  //      tft.print(file.name());
  //      tft.print("  SIZE: ");
  //      tft.println(file.size());
  //    }
  //
  //    file.close();
  //    file = root.openNextFile();
  //  }

  //Look for update.bin

  //Look for update.bin with checksum, like update_0xdfe87f09.bin
  File root = fs.open("/");
  root.rewindDirectory();
  File file = root.openNextFile();

  char* updateBinName;

  int numfiles = 0;

  while (true) {

    if (!file.isDirectory()) {


      //size has to be 10 or 21
      if (strlen(file.name()) == 11
          && strcmp(file.name(), "/update.bin") == 0
         )
      {
        //regular update file
        tft.println("Found legacy update.bin file");
        updateBinName = strdup(file.name());
        numfiles++;
      }

      if (strlen(file.name()) == 22
          && strcmp(String(file.name()).substring(0, 8).c_str(), "/update_") == 0
          && strcmp(String(file.name()).substring(18, 22).c_str(), ".bin") == 0
         )
      {
        //checksum update file
        tft.print("Checking file:");
        tft.println(file.name());
        tft.println("This will take up to two minutes, please wait");
        char crc[11];
        uint32_t len;
        sprintf(crc, "%#08x", crc32(file, len));
        if (strcmp(String(file.name()).substring(8, 18).c_str(), crc) == 0) {
          tft.print("Found valid update file with CRC: ");
          tft.println(crc);
          updateBinName = strdup(file.name());
          numfiles++;

        } else {
          tft.print("Found update file with INVALID CRC: ");
          tft.println(crc);
          tft.print("Declared CRC: ");
          tft.println(String(file.name()).substring(8, 18).c_str());
          tft.println("Cancelling update");
          delay(10000);
          delete updateBinName;
          return;
        }
      }
    }
    file.close();
    file = root.openNextFile();

    if (!file) {
      break;
    }
  }


  if (numfiles > 1) {
    tft.println("Found multiple update files on SD");
    tft.println("Cancelling update");
    delay(10000);
    delete updateBinName;
    return;
  }

  if (numfiles == 1) {

    File updateBin  = fs.open(updateBinName);

    if (!updateBin || updateBin.isDirectory()) {
      tft.print("Error opening ");
      tft.println(updateBinName);
      updateBin.close();
      delete updateBinName;
      return;
    }

    tft.println("Launching update from file:");
    tft.println(updateBinName);
    tft.println("PLEASE, DO NOT REMOVE POWER WHILE UPDATING");
    tft.println("PLEASE, DO NOT REMOVE SD CARD WHILE UPDATING");
    tft.println("");
    tft.println("PRESS FIRST AND SECOND BUTTON SIMULTANEOUSLY TO START UPDATE");
    tft.println("TURN OFF THE MODULE AND REMOVE SD CARD TO CANCEL");
    while (SPIExpanderButtons.digitalRead(0) || SPIExpanderButtons.digitalRead(1)) {

    }
    tft.println("");
    size_t updateSize = updateBin.size();

    if (updateSize > 0) {
      tft.println("Starting update... DO NOT REMOVE SD CARD NOW");
      performUpdate(updateBin, updateSize, fs, updateBinName);
    }
    else {
      tft.println("Error, file is empty");
    }

    updateBin.close();

  }
  else {
    tft.println("Could not load update file from sd root");
    delay(2000);
  }

  delete updateBinName;
}
