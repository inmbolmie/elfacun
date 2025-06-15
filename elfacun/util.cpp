#include "util.h"


//Sound
boolean soundIsDisabled = false;
boolean soundIsDisabledPassiveLed = false;


void boop() {
  if (!soundIsDisabled) {
    ledcWriteTone(0, freqTone1);
    delay(durationTone1);
    ledcWriteTone(0, 0);
  }
}


void beep() {
  if (!soundIsDisabled) {
    ledcWriteTone(0, freqTone2);
    delay(durationTone2);
    ledcWriteTone(0, 0);
  }
}


void buup() {
  if ((!soundIsDisabled) && (!soundIsDisabledPassiveLed)) {
    ledcWriteTone(0, freqTone3);
    delay(durationTone3);
    ledcWriteTone(0, 0);
  }
}
