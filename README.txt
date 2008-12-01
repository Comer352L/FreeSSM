FreeSSM
=======

FreeSSM is a free and easy to use diagnostic and adjustment tool for SUBARU®
vehicles. It currently supports the models LEGACY®, LIBERTY®, OUTBACK®, BAJA®,
IMPREZA®, FORESTER® and TRIBECA® starting with model year 1999 and provides
access to the engine and transmission control units.

PLEASE NOTE:
This program is NOT A PRODUCT OF FUJI HEAVY INDUSTRIES LTD. OR ANY SUBARU®-
ASSOCIATED COMPANY. It is a free reengineering project which is not contributed,
provided or supported by any company in any way.

All trademarks are property of Fuji Heavy Industries Ltd. or their respective
owners.

--------------------------------------------------------------------------------

0. TABLE OF CONTENTS:

   1. Licence
   2. Supported Platforms
   3. Requirements
   4. Compilation
   5. Starting FreeSSM

--------------------------------------------------------------------------------

1. LICENCE:

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
You should have received a copy of the GNU General Public License along with
this program (see file LICENCE.txt). If not, see <http://www.gnu.org/licenses/>.

The use of this program is AT YOUR OWN RISK. The author is NOT LIABLE FOR ANY
EFFECTS caused by usage, including UNEXPECTED VEHICLE BEHAVIOR or DAMAGES.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of FUNCTIONALITY ON YOUR VEHICLE,
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

--------------------------------------------------------------------------------

2. SUPPORTED PLATFORMS:

   - Linux/X11
   - Windows

With minor modifications, it should be possible to build FreeSSM on MacOS X
and other Unix systems, too, but there is currently no offical support.

--------------------------------------------------------------------------------

3. REQUIREMENTS:
   - FreeSSM source code (freessm.berlios.de)
   - Qt4 >= 4.4.0 (www.trolltech.com)
   - MinGW (only on MS Windows systems) (www.MinGW.org)

GCC 4.1.x is recommended for compilation.

--------------------------------------------------------------------------------

4. COMPILATION:

Open a console window and switch to the FreeSSM-directory.

Preparation:
$ qmake

Compilation:
$ make

Translation files:
$ make translation

--------------------------------------------------------------------------------

5. STARTING FreeSSM:

Linux:
$ ./FreeSSM

Windows:
$ freessm


