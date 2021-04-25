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
   5. Installation
   6. Starting FreeSSM

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


TinyXML is released under the zlib license, see src/tinyxml/readme.txt and
www.sourceforge.net/projects/tinyxml for further informations.

--------------------------------------------------------------------------------

2. SUPPORTED PLATFORMS:

   - Linux/X11
   - Windows (XP and older are untested/unmaintained !)

With minor modifications, it should be possible to build FreeSSM on MacOS X
and other Unix systems, too, but there is currently no offical support.

--------------------------------------------------------------------------------

3. REQUIREMENTS:
   1.) FreeSSM source code (https://github.com/Comer352L/FreeSSM)
   2.) Qt framework: Qt4, Qt5 or Qt6
        Development of Qt4 has been discontinued, the last release is v4.8.7.
        Releases older than 4.8.7 are no longer tested !
        Support for Qt6 is still experimental.
        There is no functional difference between the Qt versions with regards to FreeSSM.
        2.1.) Download sources:
               Qt4: download.qt.io/archive/qt/4.8/4.8.7/
               Qt5 and Qt6: www.qt.io/download, https://www.qt.io/download-qt-installer
               Linux distributions usually provide ready packages in their
               repositories, which should be preferred. Development packages are
               needed, too.
        2.2.) OS specifics:
               Windows: The MinGW version of Qt is needed.
                        All other versions (e.g. for MS Visual Studio) are not
                        supported.
               Linux: Distributions split the Qt framework into multiple packages.
                      Besides the core/main development package, an additional package
                      is usually required for buidling the translations:
                      On Debian based distributions, the package for Qt5 is called
                      "qttools5-dev-tools".
                      On openSUSE, the package for Qt5 is called "libqt5-linguist" while
                      for Qt6 the package is called "qt6-tools-linguist".
        2.3.) General notes:
               The sources of Qt are not required.
               There is also no need to install tools such as Qt Creator etc.
   3.) MinGW (only for MS Windows)
       MinGW is usually shipped with Qt and just needs to be selected during
       the installation process.
       However, in the past the installation process changed very often.
       There were times when it had to be downloaded and installed manually.
       If the Qt-installer is not shipping MinGW, it usually provides at least a
       direct link for downloading it. If that's not the case, check
       mingw.org and sourceforge.net/projects/mingw.
       The 32 bit version is required for building a 32 bit application while
       the 64 bit version is required for building a 64 bit application.
       Both versions can be installed in parallel.

--------------------------------------------------------------------------------

4. COMPILATION:

4.1 PRELIMINARY NOTE:

Only compilation from command line is maintained/tested (but compilation with
Qt Creator etc. might work as well).

4.2 BUILDING A 32 OR 64 BIT APPLICATION ?

If you are going to use a J2534-interface, you should check if the interface
library is available as 32 and/or 64 bit version.
32 bit libraries can only be used by a 32 bit application and 64 bit libraries
can only be used by a 64 bit application.
At least on MS Windows, most libraries are still available as 32bit version only.
So you likely prefer to build a 32 bit application here.

To build a 32 (64) bit application on MS Windows, you have to use
the 32 (64) bit version of MinGW.

4.3 COMPILATION STEPS (COMMAND LINE):

Open a console window and switch to the FreeSSM-directory.
NOTE (Windows only):
If MinGW is installed properly, there's a start menu entry which opens a command
line window and sets up the environment for compilation:
e.g. "Start" > Qt 5.12.3 > 5.12.3 > MinGW 7.3.0 (32bit) > Qt 5.12.3 (MinGW 7.3.0 32bit)

Preparation:
$ qmake
or (if you have multiple versions of Qt installed)
$ qmake-qt4   or   $ qmake-qt5   or   $ qmake6   (depending on your system environment)

If you want to build the version for small display resolutions, call qmake with
"CONFIG+=small-resolution" appended or uncomment the corresponding line in the
project file 'FreeSSM.pro' before calling qmake.

Compilation:
$ make release
or
$ make debug

Translation files:
$ make translation

NOTE (Windows only): depending on the used Qt-version and system configuration,
                     'mingw32-make' must be called instead of 'make'.

--------------------------------------------------------------------------------

5. INSTALLATION:

By default, the the application will be installed to
	Linux:        the users home-directory (/home/userXYZ/FreeSSM)
	Windows:      C:\FreeSSM
If you want to install to a custom directory, call qmake first with
"INSTALLDIR=CUSTOM_DIR" appended, e.g.:
$ qmake "INSTALLDIR=D:\somedir\FreeSSM_TEST"


Installation:

$ make release-install
or
$ make debug-install


Uninstallation:

$ make release-uninstall
or
$ make debug-uninstall

--------------------------------------------------------------------------------

6. STARTING FreeSSM from command line:

First, switch to the installation folder (see 5.).

Linux:
$ ./FreeSSM

Windows:
$ freessm

You can append additional command line options if useful.
To get a full list of supported options and corresponding descriptions, start
FreeSSM with option "-h" (or "--help").

Examples:
$ ./FreeSSM -h
=> prints help text (list of supported command line options)

$ ./FreeSSM -c transmission -f mbssws -p selectionfile="/home/users/myMBSWselection_4.list" autostart
=> starts directly into the transmission control unit dialog, selects reading of
   measuring blocks and switches, loads the MB/SW selection file
   "/home/users/myMBSWselection_4.list" and starts reading immediately
