# Hitman: Codename 47 Speed/Timing Fix

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

This project provides an unofficial binary patch for the 2000 PC game Hitman:
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
and testing. It should still work with the Steam or the CD versions.
The installer verifies the checksums so if the installation is successfull the
game will most likely work fine. Let me know if you have any problems.

## How to install?

Download the latest `patcher.exe` to the game's directory (it should be in the
same directory as `hitman.exe`) and run it by double clicking. If you're not on
Windows run it through Wine. The patcher will tell you how the patching went.

## How to uninstall?

The patcher creates a backup `.BAK` file for every files it modifies. To
uninstall, just overwrite each of the files with their corresponding backup
file.

## How to set a custom FPS?

After applying the patch, open `system.dll` in a hex editor and change the byte
at location `000112EF` to your desired FPS.

## Cause

The game uses the CPU instruction RDTSC to keep track of how much time passes
between the frames to update the state: camera, physics, sounds etc. RDTSC
returns a value which is incremented every CPU cycle and is now considered [a very
poor form of timing](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/game-timing-and-multicore-processors)
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
the frame time proportionally as described above if the game is running at more
than 45 FPS which I think provides the best feeling. It's easy to change this
constant according to your preferrence. 

## License

This project is licensed under the 3-Clause BSD License. See the
[license file](LICENSE) for the full text.

This project uses the library [md5-c](https://github.com/Zunawe/md5-c).

See [Hitman: Codename 47 @ GoG](https://www.gog.com/en/game/hitman_codename_47)
for more info on the copyright and license of the game itself.
