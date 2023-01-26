# Elfacun Chess Interface Module

![Elfacun](./images/pic7.jpg)

## Known Limitations

### General operation

Piece sliding is not recommended at this point, you should always lift and place the pieces you move. Future updates could allow or improve piece sliding support.

For taking pieces it is irrelevant if you lift first the taking or the taken piece, it will work both ways. To take back movements where a piece has been taken, you have to undo the movement before placing back the taken piece though.


### 3D-printed case

A custom 3D printed case is provided for the module. It has been designed to be mechanically similar to the original module cases, but adapted to the button and screen layout of Elfacun.

A 3D printed case is easy and cheap to produce for a small-scale project like this, but has some drawbacks:

* Aesthetically will be always worse than a injection-molded case

* The small hooks that fit together the top and bottom parts are more fragile than in the original modules. For that reason the module has an opening on top of the case for the user to have access to the SD card slot and adjustable potentiometers withoud a need to remove the module from the case. So it is better not to try to open the case, or at least being very careful if it needs to be done. In case some of the hooks snapped, it wouldn't affect the module integrity at all, because when inserted into the board slot the two parts will be held in place together anyway.



### Connectivity

Elfacun uses the on-board USB, WIFI and Bluetooth resources of the ESP32 module, so it will be able to connect to any system as far as that system is compatible with the ESP32 WIFI and Bluetooth modes. More details can be found here:

[http://esp32.net/](http://esp32.net/)

    Wi-Fi: 802.11 b/g/n/e/i (802.11n @ 2.4 GHz up to 150 Mbit/s)
    Bluetooth: v4.2 BR/EDR and Bluetooth Low Energy (BLE)



### Passive mode operation

Pasive mode is disabled while in Lichess mode, as it would be against their rules to receive computer assistance from a module while in a game.

While in passive mode it can be tricky to fix mangled board positions or take back movements. Think that you have two separate systems (Elfacun and the original module) listening to the piece movements, so anything you make will be applied equally in both sides at the same time, and can be interpreted differenty by the modules if they have different rules. So when in passive mode try to be very careful moving pieces and try not to make worng movements or drop any pieces, because that mistakes could be difficult to fix while in a game.

For pawn promotion, usually the original modules will require you to press a button to indicate the piece you promote to.

In passive mode the original module controls the board leds, so you need to get the opposing movements somewhere else, like in the app you have connected to Elfacun or in the DGT3000 clock messages if operating in DGT mode.


### Lichess mode

Lichess mode is based on the public Lichess Board API. This API has some limitations. Other limitations come from the limited resources available in the ESP32 to implement this mode. These limitations only apply to the native Lichess mode (via WIFI), so if you play Lichess through a connected app (like White Pawn) the limitations don't apply.

The Lichess Board API doesn't allow to launch games with blitz time controls. The minimum recommended time control to play on a board is 10+0 and over. If you try to launch a challenge with an invalid time control from the board, the challenge could be rejected or launched on a different time control.

The Lichess TV stream does not contain game result data, and it would be complicated to make additional requests to get that data, so no game results are displayed in Lichess.tv mode. When a game ends the module will simply start to show the next game.


### Updating software


Software update through USB is not directly supported. Part of the ESP32 module programming hardware has been disabled due to incompatibilities with some apps in Windows when connecting to them through USB. If you ever want to program the ESP32 via USB you would have to manually enter programming mode and reset the module. This should not be a problem to regular users because the module supports software update using the SD card reader.


### V2 PCB

The first revision of V2 PCB has two patched mistakes that doesn't affect the module functionality, and will be fixed in subsequent releases.


### PCB

The first version PCB has two patched mistakes that doesn't affect the module functionality, and will be fixed in subsequent releases:

* The buzzer transistor had to be reversed as it was installed in the wrong orientation
* There is a patch wire that enables the bus switches directly from +5V. It was found during testing that the original 3,3V signal was not high enough to enable them in some cases (when using on 6V board power supplies).

_"DGT", "Millennium" and "Mephisto" are trademarks of their respective owners.
All the trademarks are used nominatively to indicate compatibility, and do not indicate affiliation to or endorsement by the trademark owners._
