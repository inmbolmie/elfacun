/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "serial.h"
#include "crc32.h"

inline uint32_t updateCRC32(uint8_t ch, uint32_t crc)
{
   uint32_t idx = ((crc) ^ (ch)) & 0xff;
   uint32_t tab_value = pgm_read_dword(crc_32_tab + idx);  
   return tab_value ^ ((crc) >> 8);
}

uint32_t crc32(File &file, uint32_t &charcnt)
{
      uint32_t oldcrc32 = 0xFFFFFFFF;
      charcnt = 0;

      while (file.available())
      {
        uint8_t c = file.read();
        charcnt++;
        oldcrc32 = updateCRC32(c, oldcrc32);
      }

      return ~oldcrc32;
}
