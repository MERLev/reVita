# reVita v.1.0
**reVita** is a plugin for PS Vita / PS TV, which allows you to remap inputs and trigger different actions. It is a continuation of [Rinnegatamante](https://github.com/Rinnegatamante)'s **[remaPSV](https://github.com/Rinnegatamante/remaPSV)**, which was fully rewritten as a kernel plugin.

![Image](/include/screenshot.png)

## Functionality

- Remap
  - buttons (combos too)
  - analog sticks
  - back and front touch zones
  - gyroscope movements
- Emulate 
  - buttons (combos too)
  - analog sticks, 
  - touch presses and swipes
- Trigger system events
  - Reboot
  - Sleep
  - Power Off
  - Display Off
  - Kill current application
  - Brightness control (works on PS TV too)
  - Savegames backup/restore
- swap touchpads
- display native and emulated touch pointers
- disable Gyro's deadband for better sensivity
- virtual DS4 controller
- improve gyro sensitivity in any game by disabling gyro deadband
- external controllers support
- per-game and global profiles
- themes

## Compability with plugins

- **[MiniVitaTV](https://github.com/TheOfficialFloW/MiniVitaTV)** - full compability
- **[ds34vita](https://github.com/MERLev/ds34vita)** - full compability
- **[ds4touch](https://github.com/MERLev/ds4Touch)** - full compability
- **[ds3vita](https://github.com/xerpi/ds3vita)** and **[ds4vita](https://github.com/xerpi/ds4vita)** - partial compability, use **[ds34vita](https://github.com/MERLev/ds34vita)** instead to get full support.
- **[DSMotion](https://github.com/OperationNT414C/DSMotion)** - not compatible, use **[DS34Motion](https://github.com/MERLev/DS34Motion)** instead to get full support.

## Compability with Apps

- Not compatible with most system apps and some of the homebrew.
- Adrenaline - to get UI working, you need to change **Adrenaline Settings -> Graphics Filtering** to anything else **except** original.

## Installation

- Copy **ioplus.skprx** to **ur0:/tai** folder, add **ioplus.skprx** into your **ur0:/config.ini** config file under ***KERNEL*** section.
- Copy **reVita.skprx** to **ur0:/tai** folder, add **reVita.skprx** into your **ur0:/config.ini** config file under ***KERNEL*** section.
- [Optional, to get Gyro support] Copy **reVitaMotion.suprx** to **ur0:/tai** folder, add **reVitaMotion.suprx** into your **ur0:/config.ini** config file under ***MAIN*** section.

## Usage

- To bring the config menu, press **START + SQUARE**, customisable under Settings -> Hotkeys.

## FAQ

- How to open plugin menu ?
  - Press (start) + (square)
- What can you do with gyro ?
  - You can remap gyro direction (up, down, left, right) to right stick directions to enable gyro aim in any game.
- Is there a way to turn it on and off?
  - **START + TRIANGLE**, customisable under Settings -> Hotkeys
- Does it save presets on a game by game basis?
  - Yes, you can use Profile -> Profile Management for more options.
- What is deadband under gyro menu ?
  - Deadband limits gyroscope sensitivity, so smallest movements are ignored to handle shaking hands, and it is enabled by default in most vita games. Disabling it will higly increase sensitivity of gyroscope.

## Known issues

- Menu not opening in Adrenaline
  - Change **Adrenaline Settings -> Graphics Filtering** to anything else **except** original.

## Credits

Thanks to evryone who helped me along the way :
- [bosshunter](https://github.com/bosshunter), for doing most of the testing and supplying me with ideas.
- [Rinnegatamante](https://github.com/Rinnegatamante), [remaPSV](https://github.com/Rinnegatamante/remaPSV) author, for various help provided.
- [S1ngyy](https://github.com/S1ngyy), for providing code for analogs/gyro support
- [pablojrl123](https://github.com/pablojrl123), for customizable buttons activation code.
- [Bythos](https://github.com/bythos14), for help with reversing, fixing libk and other general stuff.
- [ellipticaldoor](https://github.com/ellipticaldoor) for testing.
- [teakhanirons](https://github.com/teakhanirons) for various help and advices.
- [Princess-of-Sleeping](https://github.com/Princess-of-Sleeping) for help with reversing.
- Derpy (Cassie) for testing.
- W0lfwang for testing.
- TheIronUniverse for testing.
- mantixero for testing it in PS4link.
- Kiiro Yakumo for testing it in PS4Link.
- Nino1026 for testing.
- [Vita Nuova](https://t.co/3Efi3PGwK5?amp=1) communinity for all the help and support I got there.
- [HENkaku](https://discord.gg/m7MwpKA) communinity for various help.

## Original credits from [remaPSV](https://github.com/Rinnegatamante/remaPSV) by [Rinnegatamante](https://github.com//Rinnegatamante)

- Dmaskell92 for testing the plugin.
- All my Patroners for their awesome support:
- nobodywasishere
- RaveHeart
- Tain Sueiras
- 2Mourty
- Andyways
- ArkaniteOnVita
- Artūrs Lubāns
- BOBdotEXE
- ckPRO
- Count Duckula
- Daniel
- Eduardo Minguez
- Elwood Blues
- Gelson Silva
- Igor Kovacs Biscaia
- Jared Breland
- Lars Zondervan
- Mored1984
- gnmmarechal
- Oleg Des
- Pirloui
- rsn8887
- Samuel Batista
- styroteqe
- suLac4ever
- T33Hud
- Troy Murray
- Yakara Network
- PSX-Place.com
