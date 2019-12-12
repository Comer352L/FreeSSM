## Easy compilation and run at Windows & QT 4.8.7
### Prepare Compile Environment
- Download latest and suitable Mingw from https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/

- Extract or install Mingw to c:\mingw32

Download QT 4.8.7 from http://download.qt.io/archive/qt/4.8/4.8.7/qt-opensource-windows-x86-mingw482-4.8.7.exe
- Setup QT

- Download FreeSSM source code files from https://github.com/Comer352L/FreeSSM

Extract source code files to working directory. For example d:\freessm

## Compiling

- Run "Qt 4.8.7 Command Prompt" from "Start Menu" --> "Program Files" --> "Qt 4.8.7 (MinGW 4.8.2 OpenSource)"


- Change Directory to "d:\freessm"

    d:
    cd \freessm

- Prepare source code files : 

    qmake

- Compile source code:

    make release-install

- Make and copy translation files to install directory c:\freessm : 

    make translation
    copy *.qm c:\freessm\

- Replace and overwrite libstdc++-6.dll file. Because automatic installed file gives an error:

    copy /Y "C:\mingw32\bin\libstdc++-6.dll" c:\FreeSSM

- Finished.
   
   You can run from c:\freessm\ or copy whole FreeSSM directory to USB or another media.
