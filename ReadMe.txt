WKB Layout 2.02 README file
===========================


1. About
--------
WKB Layout is a simple keyboard layout switcher for Windows. The modifier
keys (left/right Control, Alt, Shift and Windows) may be used to select the
next, previous, or a specific keyboard layout. Any assigned keys may still
be used in combinations, such as Ctrl+C, Alt-Click etc. They will work as
standalone keys in some games as well.

After adding or removing a keyboard from Windows Settings or Control Panel,
you will need to restart WKB Layout, or invoke and confirm WKB Settings.


2. Antivirus warnings.
----------------------
As of today (2022-11-21), 1 out of 72 virustotal antivirus engines flags
the WKB Layout installer as "malicious". This is a common occurense for
software packaged with NSIS. If you have suspicions, get the source code
and compile it yourself (see p.4 below), or ask somebody to do so.


3. User accounts.
-----------------
WKB Layout does not work with programs started "as administrator", unless
it's also started as administrator, for example by the installer. It does
not work in the Login, Lock, UAC and Ctrl-Alt-Del screens, though Scroll
Lock light may remain on, which is harmless.

Whether WKB will work in Windows/File Explorer started "as administrator"
depends on your Windows settings.


4. Compilation.
---------------
To compile WKB Layout, you will need:

- MSYS2 with GNU make and mingw-w64-x86_64-gcc
- Nullsoft Install System (NSIS), version 3.04 (recommended) or later

Compile the C code with make, and then install.nsi with NSIS. Done.

mingw-w64-i686-gcc or even Visual C++ may also be used, but these are not
officially supported.

Compiling software is not a trivial process, so if you have problems, please
consult with a specialist.


5. Compatibility.
-----------------
Requires Windows 7 or later, 64-bit.
Tested with Windows 7 SP1 and Windows 10.
Tested with universal (Metro/Modern) applications.
Tested with Windows 10 and Sysinternals virtual desktops.
Not tested with NVidia virtual desktops, they seem obsolete.
Not tested with Windows 8, 11, and server operating systems.


6. Notes.
---------
With the indicator enabled, the Scroll Lock key should work normally, but
the Scroll Lock state will reflect that of the indicator.

WKB Layout will not disable the standard layout change keys, and will not
alter any Windows settings.

When using virtual machines and remote connection applications, the host and
remote/guest Scroll Lock indicators may be conflicting. Disable one of them.

With Sysinternals Desktops, WKB Layout must be started/stopped separately
for each desktop.


7. Changes.
-----------
0.92: Removed the experimental service/accesibility code. Initial release.
0.94: Restored the Scroll Lock key functionality, except for the state.

1.06: Rewritten for compatibility with 64-bit systems, common dialogs etc.
1.07: Fixed Start Menu -> Start, Stop, Settings from a non-default desktop.
1.08: Ignore W10 error 170 on (un)install or when started as administrator.
1.09: Option to unload the wrong lock/unlock, Ctrl-Alt-Del etc. layouts.
1.10: Added Caps Lock and Print Screen to the list of toggle/shift keys.
1.11: Simpler unload, display unload error message on the starting desktop.

2.00: Rewritten and simplified. Windows 7+ 64-bit only. Modifier keys only.
2.01: Shift-Change layout replaced with exact layout keys. GPL3+ license.
2.02: Use IMM/thread windows. 64-bit only C code (compilable as 32-bit).


8. Legal information.
---------------------
WKB Layout version 2.02, Copyright (C) 2016-2022 Dimitar Toshkov Zhekov.
Report bugs to <dimitar.zhekov@gmail.com>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc., 51
Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

The gucharmap1 icon is based on (and almost identical to) gucharmap.svg,
created August 2007 by Andreas Nilsson, under GPL.
