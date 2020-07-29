# remaPSV2 v.2.0.2
**remaPSV2** is an updated version of [Rinnegatamante](https://github.com/Rinnegatamante)'s **[remaPSV](https://github.com/Rinnegatamante/remaPSV)** (which takes the name from the glorious remaPSP included in cwCheat for PSP). 

![Image](/include/screenshot.png)

## Functionality

- ability to remap buttons, analog sticks, back and front touch zones, gyroscope movements
- ability to emulate buttons, analog sticks, touch events
- 4 pre-defined and 4 customizable touch points to emulate
- improve gyro sensivity in any game by disabling gyro deadband
- external controllers support
- per-game and global profiles

## Compability

- Full support
  - All official games and some of the homebrew
- Partial support without plugin menu opening [[How To]](https://github.com/MERLev/remaPSV2#known-issues)
  - Adrenaline, PS4link, some of the homebrew
- No support
  - Livearea, all other system apps, some of the homebrews

## Installation

- Copy **remaPSV2.suprx** and **ioplus.skprx** to **ur0:/tai** folder 
- Add **remaPSV2.suprx** into your **ur0:/config.ini** file (under ***ALL** or whatever game you want to use it).
- Add **ioplus.skprx** into your taiHen config file (under ***KERNEL**)
- To bring the config menu, press **START + Square** in game.

## Usage

- When in-game, press (start) + (square) to bring up plugin menu

## FAQ

- How to open plugin menu ?
  - Press (start) + (square) whin in-game
- What can you do with gyro ?
  - You can remap gyro direction (up, down, left, right) to right stick directions to enable gyro aim in any game.
- How do I map combo of buttons ?
  - Combo mappings are not supported
- Is it possible to remap PS4Link keys
  - Yes, but a bit [tricky](https://github.com/MERLev/remaPSV2#known-issues)
- Is there a way to turn it on and off? Or does it save presets on a game by game basis?
  - ATM, no
- Is there any way to have this work on the save screen/system apps/livearea ?
  - ATM, no
- What is deadband under gyro menu ?
  - Deadband limits gyroscope sensivity, so smallest movements are ignored to handle shaking hands, and it is enabled by default in most vita games. Disabling it will higly increase sensivity of gyroscope.


## Known issues

- Menu not opening in Adrenaline, PS4link and some homebrew. 
  - Solution: To use plugin for such apps, you can follow those steps
    1. Open any game/app where plugin menu is working
    2. Configure everything as of doing it for desired app with menu not working.
    3. Go to settings -> save as Global config
    4. Open you desired app -> it should already be using global config and you remaps should be working
    5. Press (START) + (TRIANGLE) to save your global profile as current running game profile.
  
- Unresponsive black screen after going out of sleep mode. 
  - Solution: Should be fixed in 2.0.1 version.

- Screen blinking in Jak and Daxter. 
  - Solution: restart an app.

## Credits

- [original version](https://github.com/Rinnegatamante/remaPSV) created by [Rinnegatamante](https://github.com/Rinnegatamante)
- [S1ngyy](https://github.com/S1ngyy), for providing code for analogs/gyro support
- [pablojrl123](https://github.com/pablojrl123), for customizable buttons activation code
- Cassie for testing
- W0lfwang for testing
- TheIronUniverse for testing
- mantixero for testing it in PS4link
- Kiiro Yakumo for testing it in PS4Link
- [Vita Nuova](https://t.co/3Efi3PGwK5?amp=1) communinity for all the help and support I got there

## Original credits by [Rinnegatamante](https://github.com//Rinnegatamante)

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
