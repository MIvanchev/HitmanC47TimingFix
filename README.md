# Hitman: Codename 47 Speed/Timing Fix

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Build status](https://ci.appveyor.com/api/projects/status/s96nc560pf8pjdd2?svg=true)](https://ci.appveyor.com/project/MIvanchev/hitmanc47timingfix)

This project provides an **unofficial** binary patch for the 2000 PC game Hitman:
Codename 47 which fixes numerous issues.

Use at your own risk and discretion. I assume no responsibility for any damage
resulting from using this project.

Please report any bugs and issues with the patch so I can make it better. It
is considered work in progress until I get some feedback on how it behaves in
the wild.

## What's already fixed?

* Game running too fast on modern hardware
* Floating guns at high FPS

## What's still being fixed?

* Hor+ widescreen support
* Readable in-game GUI across all resolutions
* Camera sway at high FPS
* Excessive rag doll physics at high FPS

## Supported versions of the game

This patch is exclusively for version b192 of the game as reported in the game's
main menu. I have exclusively used the GOG version of the game for development
and testing but other distributions are supported as well. The installer
verifies the checksums so if the installation is successfull the game will most
likely work fine. Let me know if you have any problems.

## How to install?

From the table below, download the latest patcher for your distribution to
the game's main directory (directory containing `hitman.exe`)
and lauch it by double clicking. You might have to run it as an administrator.
If you're not on Windows, simply run it through Wine. The patcher will tell you
how the patching went.

| Game distribution | Patcher |
| ----------------- | ------- |
| GoG, version b192 | [patcher-gog](https://ci.appveyor.com/api/buildjobs/ihhqrtd2kf9lyoc0/artifacts/patcher-gog.exe) |
| Steam & CD, version b192 | [patcher-other](https://ci.appveyor.com/api/buildjobs/ihhqrtd2kf9lyoc0/artifacts/patcher-other.exe) |

## How to uninstall?

The patcher creates a ZIP file called `patch-backup.zip` containing the original
game files. To uninstall, just extract the archive in the game's main directory.

## How to use?

After applying the patch, start the game and see if the speed is now OK. If it's
still too fast, open `hitman.ini` and add the line `Fps <n>` where
`<n>` is a number lower than your actual FPS. You'll have to experiment to
find a good value. For instance my Hitman runs somewhat too fast on 60 FPS
and setting `Fps 50` makes it nicely playable. Setting `Fps 0` or a negative
value has no effect, i.e. the game's speed will not be modified.

The patch uses time stretching to slow the game down. This means that when you
set `Fps <n>`, 1 game second will equal 1+ actual seconds and the individual
frame times will be low. Unfortunately, Hitman: Codename 47 suffers from
numerous problems at low frame times which might be triggered if the time
is stretched too much, for example Matrix-like rag doll effects or excessive
camera sway. I'm working on fixing those as well.

## How to build?

First you need to generate the header with the patches and their locations for
the different game distribuitions (GoG, Steam, CD). Do this by
`./compile-patch.sh > patches.h`. The compile the patcher with MinGW for the
distribution you want by
`mingw32-gcc -mwindows -DPATCH_FOR_GOG -o patch-gog.exe src\patcher\*.c`.
Look at the
[AppVeyor file](https://raw.githubusercontent.com/MIvanchev/HitmanC47TimingFix/main/appveyor.yml)
for more info.

## License

This project is licensed under the 3-Clause BSD License. See the
[license file](LICENSE) for the full text.

This project uses the libraries [md5-c](https://github.com/Zunawe/md5-c) and
[miniz](https://github.com/richgel999/miniz).

See [Hitman: Codename 47 @ GoG](https://www.gog.com/en/game/hitman_codename_47)
for more info on the copyright and license of the game itself.
