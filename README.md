# Hitman: Codename 47 Speed/Timing Fix

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Build status](https://ci.appveyor.com/api/projects/status/s96nc560pf8pjdd2?svg=true)](https://ci.appveyor.com/project/MIvanchev/hitmanc47timingfix)

This project provides an **unofficial** binary patch for the 2000 PC game Hitman:
Codename 47 which fixes the problem that the game runs too fast on modern
hardware. The mechanics of the patch are explained for fellow hackers and
technically-inclined persons.

Use at your own risk and discretion. I assume no responsibility for any damage
resulting from using this project.

Please report any bugs and issues with the patch so I can make it better. It
is considered work in progress until I get some feedback on how it behaves in
the wild.

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
| GoG, version b192 | [patcher-gog](https://ci.appveyor.com/api/buildjobs/noc5rq0ebk4d2evc/artifacts/patcher-gog.exe) |
| Steam & CD, version b192 | [patcher-other](https://ci.appveyor.com/api/buildjobs/noc5rq0ebk4d2evc/artifacts/patcher-other.exe) |

## How to uninstall?

The patcher creates a backup `.BAK` file for every files it modifies. To
uninstall, just overwrite each of the files with their corresponding backup
file.

## How to use?

After applying the patch, open `hitman.ini` and add the line `Fps <n>` where 
`<n>` is a number lower than your actual FPS. You'll have to experiment to find
a good value. `Fps 0` has no effect, i.e. the game runs at the actual FPS.

## How to build?

First you need to generate the header with the patches and their locations for
the different game distribuitions (GoG, Steam, CD). Do this by
`./compile-patch.sh > patches.h`. The compile the patcher with MinGW for the
distribution you want by
`mingw32-gcc -mwindows -DPATCH_FOR_GOG -o patch-gog.exe src\patcher\*.c`.
Look at the
[AppVeyor file](https://raw.githubusercontent.com/MIvanchev/HitmanC47TimingFix/main/appveyor.yml)
for more info.

## Cause

The game uses the CPU instruction RDTSC to keep track of how much time passes
between the frames to update the state: camera, physics, sounds etc. RDTSC
returns a value which is incremented every CPU cycle and is now considered
[a very poor form of timing](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/game-timing-and-multicore-processors)
for numerous reasons, one being that the frequency of modern CPUs is not
constant. The recommended way of obtaining timestamps on Windows is through the
API functions [QueryPerformanceFrequency](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency)
and [QueryPerformanceCounter](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter).

Hitman: Codename 47 is also not FPS limited. This means that the more times per
second the game updates its state and renders the faster it appears to run.
Back in 2000 the developers weren't considering that 20 years later the game
will run at hundeds of frames. There are numerous strategies to address the
problem, for example playing on huge resolutions and enabling all graphical
features to slow the game down or throttling the CPU/GPU.

Another way which doesn't limit the FPS but solves the speed issue
is to make the game think less time is passing between the frames than actually
is. Suppose the game is designed to run at 60 FPS but is running at 120 and
feels twice as fast. The frame time will be 1/120 seconds or 8ms. If we reduce
the frame time twice to 1/240 seconds or 4ms and use this for the state updates
the game will be slowed down to its originally intended speed.

## Solution

The patch changes the game's code to use QueryPerformanceFrequency and 
QueryPerformanceCounter instead of RDTSC for obtaining timestamps and scales
the frame time proportionally as described above if the game is running too
fast at the current FPS.

## License

This project is licensed under the 3-Clause BSD License. See the
[license file](LICENSE) for the full text.

This project uses the library [md5-c](https://github.com/Zunawe/md5-c).

See [Hitman: Codename 47 @ GoG](https://www.gog.com/en/game/hitman_codename_47)
for more info on the copyright and license of the game itself.
