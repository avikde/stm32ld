# STM32 serial flash tool with auto-reset

* Burn .bin (no hex support) firmware images to STM32 microcontrollers using the built-in serial bootloader.
* This version automatically toggles DTR and RTS to try to auto-reset into bootloader mode. Currently works for BOOT0 = RTS, NRST = DTR, but the flipped version is a few easy modifications away.

## Usage

```
$ stm32ld <port> <baud> <binary image name|0 not to flash> [<0|1 to go to new flashed app>]
```

## Building

On Windows (tested with Cygwin / Mingw), Mac and Linux:
```
make
```

Auto-reset support, and other changes: Avik De <avikde@gmail.com>

Original source author: Bogdan Marinescu <bogdan.marinescu@gmail.com>
