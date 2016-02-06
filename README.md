# FreeSSM

FreeSSM is a free and easy to use diagnostic and adjustment tool for SUBARU®
vehicles. It currently supports the models LEGACY®, LIBERTY®, OUTBACK®, BAJA®,
IMPREZA®, FORESTER® and TRIBECA® starting with model year 1999 and provides
access to the engine and transmission control units.

**PLEASE NOTE:**
This program is NOT A PRODUCT OF FUJI HEAVY INDUSTRIES LTD. OR ANY SUBARU®-
ASSOCIATED COMPANY. It is a free re-engineering project which is not contributed,
provided or supported by any company in any way.

All trademarks are property of Fuji Heavy Industries Ltd. or their respective
owners.

---

## 0. Table of Contents

1.	[License](#license)
2.	[Supported Platforms](#platforms)
3.	[Requirements](#requirements)
4.	[Compilation](#compilation)
5.	[Installation](#installation)
6.	[Starting FreeSSM](#starting)

---

## <a name="license"></a> 1. License

This program is free software: you can redistribute it and/or modify it under
the terms of the **GNU General Public License** as published by the
[Free Software Foundation](http://fsf.org/),
either **version 3** of the License, or (at your option) any later version.
You should have received a copy of the GNU General Public License along with
this program (see file [LICENSE.txt](LICENSE.txt)). If not, see <http://www.gnu.org/licenses/>.

**The use of this program is AT YOUR OWN RISK!** The author is NOT LIABLE FOR ANY
EFFECTS caused by usage, including UNEXPECTED VEHICLE BEHAVIOR or DAMAGES.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of FUNCTIONALITY ON YOUR VEHICLE,
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.


**TinyXML** is released under the **zlib license**, see [src/tinyxml/readme.txt](src/tinyxml/readme.txt) and
<http://www.sourceforge.net/projects/tinyxml> for further information.

---

## <a name="platforms"></a> 2. Supported Platforms

- Linux/X11
- Windows

With minor modifications, it should be possible to build FreeSSM on MacOS X
and other Unix systems, too, but there is currently no official support.

---

## <a name="requirements"></a> 3. Requirements

1.	**FreeSSM source code** (<https://github.com/Comer352L/FreeSSM>)

2.	**Qt framework** >= 4.4.0 (<http://www.qt.io/>)
	- Qt 4.8.5 (the latest and last release of the Qt4 framework) is recommended
	- Qt 5 support is still experimental
	- for Windows, the MinGW-version is required (not the VS version)

3.	**MinGW** ("Minimalist GNU for Windows", only for MS Windows; usually shipped with Qt; see <http://www.mingw.org>)

	PLEASE NOTE:

	 The Qt 4.8.5 installer currently available at <http://www.qt.io/> does not
	 include MinGW anymore.
	 Qt 4.8.x requires MinGW with g++ 4.4, which is unfortunately no longer
	 available at <http://www.mingw.org> but can be downloaded from:
		<https://piece-of-c.googlecode.com/files/MinGW-gcc440_1.zip>
	Newer MinGW versions will not work with the binary release of Qt 4.8.x

---

## <a name="compilation"></a> 4. Compilation

Open a console window and switch to the FreeSSM-directory.

### Preparation
$ `qmake`

### Compilation
$ `make release`

or

$ `make debug`

### Translation files
$ `make translation`

NOTE (Windows only):
depending on the used Qt-version and system configuration, `mingw32-make` must be called instead of `make`.

---

## <a name="installation"></a> 5. Installation

$ `make release-install`

or

$ `make debug-install`

### Location
The application will be installed to

- Linux: the users home-directory (`/home/userXYZ/FreeSSM`)
- Windows:	`C:\FreeSSM` (can be moved after installation)

### Uninstall

$ `make release-uninstall`

or

$ `make debug-uninstall`

---

## <a name="starting"></a> 6. Starting FreeSSM

First, switch to the installation folder (see [5. Installation](#installation)).

### Linux
$ `./FreeSSM`

### Windows
$ `freessm`
