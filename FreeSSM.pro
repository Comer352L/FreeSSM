
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
CONFIG += debug_and_release	# warning: specifying EITHER release OR debug breaks the dll installation target on Windows !
TEMPLATE = app
TARGET = FreeSSM
DESTDIR = ./
DEPENDPATH += . src ui
INCLUDEPATH += . src src/tinyxml


# Input
HEADERS += src/FreeSSM.h \
           src/Languages.h \
           src/EngineDialog.h \
           src/TransmissionDialog.h \
           src/ABSdialog.h \
           src/CruiseControlDialog.h \
           src/AirConDialog.h \
           src/Preferences.h \
           src/About.h \
           src/FSSMdialogs.h \
           src/ActuatorTestDlg.h \
           src/AbstractDiagInterface.h \
           src/ATcommandControlledDiagInterface.h \
           src/SerialPassThroughDiagInterface.h \
           src/J2534DiagInterface.h \
           src/J2534.h \
           src/J2534misc.h \
           src/SSMP1communication.h \
           src/SSMP1communication_procedures.h \
           src/SSMP1base.h \
           src/SSMP2communication.h \
           src/SSMP2communication_core.h \
           src/SSMprotocol.h \
           src/SSMprotocol1.h \
           src/SSMprotocol2.h \
           src/AddMBsSWsDlg.h \
           src/ControlUnitDialog.h \
           src/CUinfo_Engine.h \
           src/CUinfo_Transmission.h \
           src/CUinfo_simple.h \
           src/CUcontent_DCs_abstract.h \
           src/CUcontent_DCs_engine.h \
           src/CUcontent_DCs_twoMemories.h \
           src/CUcontent_DCs_stopCodes.h \
           src/CUcontent_MBsSWs.h \
           src/CUcontent_MBsSWs_tableView.h \
           src/CUcontent_Adjustments.h \
           src/CUcontent_sysTests.h \
           src/DiagInterfaceStatusBar.h \
           src/SSM1definitionsInterface.h \
           src/SSM2definitionsInterface.h \
           src/SSMCUdata.h \
           src/SSMprotocol2_ID.h \
           src/SSMprotocol2_def_en.h \
           src/SSMprotocol2_def_de.h \
           src/ClearMemoryDlg.h \
           src/libFSSM.h \
           src/tinyxml/tinyxml.h \
           src/tinyxml/tinystr.h

SOURCES += src/main.cpp \
           src/FreeSSM.cpp \
           src/EngineDialog.cpp \
           src/TransmissionDialog.cpp \
           src/ABSdialog.cpp \
           src/CruiseControlDialog.cpp \
           src/AirConDialog.cpp \
           src/Preferences.cpp \
           src/About.cpp \
           src/FSSMdialogs.cpp \
           src/ActuatorTestDlg.cpp \
           src/AbstractDiagInterface.cpp \
           src/ATcommandControlledDiagInterface.cpp \
           src/SerialPassThroughDiagInterface.cpp \
           src/J2534DiagInterface.cpp \
           src/J2534misc.cpp \
           src/SSMP1communication.cpp \
           src/SSMP1communication_procedures.cpp \
           src/SSMP1base.cpp \
           src/SSMP2communication.cpp \
           src/SSMP2communication_core.cpp \
           src/SSMprotocol.cpp \
           src/SSMprotocol1.cpp \
           src/SSMprotocol2.cpp \
           src/AddMBsSWsDlg.cpp \
           src/ControlUnitDialog.cpp \
           src/CUinfo_Engine.cpp \
           src/CUinfo_Transmission.cpp \
           src/CUinfo_simple.cpp \
           src/CUcontent_DCs_abstract.cpp \
           src/CUcontent_DCs_engine.cpp \
           src/CUcontent_DCs_twoMemories.cpp \
           src/CUcontent_DCs_stopCodes.cpp \
           src/CUcontent_MBsSWs.cpp \
           src/CUcontent_MBsSWs_tableView.cpp \
           src/CUcontent_Adjustments.cpp \
           src/CUcontent_sysTests.cpp \
           src/DiagInterfaceStatusBar.cpp \
           src/SSM1definitionsInterface.cpp \
           src/SSM2definitionsInterface.cpp \
           src/SSMCUdata.cpp \
           src/SSMprotocol2_ID.cpp \
           src/SSMprotocol2_def_en.cpp \
           src/SSMprotocol2_def_de.cpp \
           src/ClearMemoryDlg.cpp \
           src/libFSSM.cpp \
           src/tinyxml/tinyxml.cpp \
           src/tinyxml/tinystr.cpp \
           src/tinyxml/tinyxmlerror.cpp \
           src/tinyxml/tinyxmlparser.cpp

SMALL_RESOLUTION {
FORMS += ui/small/FreeSSM.ui \
         ui/small/ControlUnitDialog.ui \ 
         ui/small/Preferences.ui \ 
         ui/small/CUcontent_DCs_engine.ui \
         ui/small/CUcontent_MBsSWs.ui \
         ui/small/CUcontent_MBsSWs_tableView.ui \
         ui/small/AddMBsSWsDlg.ui \
         ui/small/CUinfo_Engine.ui \
         ui/small/CUinfo_Transmission.ui \
         ui/small/CUinfo_simple.ui \ 
         ui/small/CUcontent_Adjustments.ui \ 
         ui/small/CUcontent_DCs_twoMemories.ui \
         ui/small/CUcontent_DCs_stopCodes.ui \
         ui/small/CUcontent_sysTests.ui \

} else {
FORMS += ui/FreeSSM.ui \
         ui/ControlUnitDialog.ui \ 
         ui/Preferences.ui \
         ui/CUcontent_DCs_engine.ui \ 
         ui/CUcontent_MBsSWs.ui \
         ui/CUcontent_MBsSWs_tableView.ui \
         ui/AddMBsSWsDlg.ui \
         ui/CUinfo_Engine.ui \
         ui/CUinfo_Transmission.ui \
         ui/CUinfo_simple.ui \ 
         ui/CUcontent_Adjustments.ui \ 
         ui/CUcontent_DCs_twoMemories.ui \
         ui/CUcontent_DCs_stopCodes.ui \
         ui/CUcontent_sysTests.ui \
}

FORMS +=   ui/About.ui \
           ui/ActuatorTestDlg.ui \


RESOURCES += resources/FreeSSM.qrc

TRANSLATIONS = FreeSSM_en.ts \
               FreeSSM_de.ts

translation.commands = lrelease FreeSSM.pro & $$QMAKE_QMAKE     # qmake needs to be called again, otherwise *.qm file will not be installed
QMAKE_EXTRA_TARGETS += translation

DEFINES += TIXML_USE_STL
# Add pre-processor-define if we compile as debug:
CONFIG(debug, debug|release): DEFINES += __FSSM_DEBUG__ __SERIALCOM_DEBUG__ __J2534_API_DEBUG__
# Enable stuff which is deprectaed since Qt5:
greaterThan(QT_MAJOR_VERSION, 4): DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040000	# only needed for method QHeaderView::setResizeMode()

# disable gcse-optimization (regressions with gcc-versions >= 4.2)
QMAKE_CXXFLAGS += -fno-gcse          # disable gcse-optimization (regressions with gcc-versions >= 4.2)
# language standard; requires c++11: range-for loop, constexpr, auto, initializer list, lambda expression, std::array, ...
QMAKE_CXXFLAGS += -std=c++11

# Installation
unix:INSTALLDIR = $$system(echo ~)/FreeSSM
win32:INSTALLDIR = $$system(echo %homedrive%)/FreeSSM
target.path = $$INSTALLDIR
filestarget.path = $$INSTALLDIR
filestarget.files = background.png LiberationSans*.ttf *.qm
unix:filestarget.files += resources/icons/freessm/48x48/FreeSSM.png
doctarget.path = $$INSTALLDIR/doc
doctarget.files = doc/*
defstarget.path = $$INSTALLDIR/definitions
defstarget.files = definitions/SSM1defs_*.xml
win32 {
  platformstarget.path = $$INSTALLDIR/platforms
  dllstarget.path = $$INSTALLDIR
  lessThan(QT_MAJOR_VERSION, 5) {
    # Qt4
    dllstarget.files =                                  $$[QT_INSTALL_BINS]/mingwm10.dll \
                                                        $$[QT_INSTALL_BINS]/libgcc_s_dw2-1.dll
    CONFIG(release, debug|release): dllstarget.files += $$[QT_INSTALL_BINS]/QtCore4.dll \
                                                        $$[QT_INSTALL_BINS]/QtGui4.dll
    CONFIG(debug, debug|release): dllstarget.files   += $$[QT_INSTALL_BINS]/QtCored4.dll \
                                                        $$[QT_INSTALL_BINS]/QtGuid4.dll
  } else {
    # Qt5
    dllstarget.files =                                  $$[QT_INSTALL_BINS]/libgcc_s_dw2-1.dll \
                                                        $$[QT_INSTALL_BINS]/libwinpthread-1.dll \
                                                        $$[QT_INSTALL_BINS]/libstdc++-6.dll \
                                                        $$[QT_INSTALL_BINS]/icuin51.dll \
                                                        $$[QT_INSTALL_BINS]/icuuc51.dll \
                                                        $$[QT_INSTALL_BINS]/icudt51.dll
    CONFIG(release, debug|release): dllstarget.files += $$[QT_INSTALL_BINS]/Qt5Core.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5Gui.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5PrintSupport.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5Widgets.dll
    CONFIG(debug, debug|release): dllstarget.files   += $$[QT_INSTALL_BINS]/Qt5Cored.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5Guid.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5PrintSupportd.dll \
                                                        $$[QT_INSTALL_BINS]/Qt5Widgetsd.dll

    CONFIG(release, debug|release): platformstarget.files = $$[QT_INSTALL_PLUGINS]/platforms/qwindows.dll
    CONFIG(debug, debug|release): platformstarget.files   = $$[QT_INSTALL_PLUGINS]/platforms/qwindowsd.dll
  }
}
unix {
    defstarget.files += definitions/J2534libs.xml
}
INSTALLS += target doctarget defstarget filestarget
win32:INSTALLS += dllstarget platformstarget




# OS-specific options
unix {
       DEPENDPATH += src/linux
       INCLUDEPATH += src/linux
       HEADERS += src/linux/serialCOM.h \
                  src/linux/TimeM.h \
                  src/linux/J2534_API.h
       SOURCES += src/linux/serialCOM.cpp \
                  src/linux/TimeM.cpp \
                  src/linux/J2534_API.cpp
       LIBS += -ldl -lrt
}

win32 {
       CONFIG(debug, debug|release): CONFIG += console
       DEPENDPATH += src/windows
       INCLUDEPATH += src/windows
       HEADERS += src/windows/serialCOM.h \
                  src/windows/TimeM.h \
                  src/windows/J2534_API.h
       SOURCES += src/windows/serialCOM.cpp \
                  src/windows/TimeM.cpp \
                  src/windows/J2534_API.cpp
       RC_FILE = resources/FreeSSM_WinAppIcon.rc
}
