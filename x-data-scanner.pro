QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common_tools/common_tool_func.cpp \
    config_recorder/uiconfigrecorder.cpp \
    logger/logger.cpp \
    loginwidget.cpp \
    main.cpp \
    mainmenubtnswidget.cpp \
    mainwindow.cpp \
    scanwidget.cpp \
    selfcheckwidget.cpp \
    sysconfigs/sysconfigs.cpp \
    version_def/version_def.cpp

HEADERS += \
    common_tools/common_tool_func.h \
    config_recorder/uiconfigrecorder.h \
    logger/logger.h \
    loginwidget.h \
    mainmenubtnswidget.h \
    mainwindow.h \
    scanwidget.h \
    selfcheckwidget.h \
    sysconfigs/sysconfigs.h \
    version_def/version_def.h

FORMS += \
    loginwidget.ui \
    mainmenubtnswidget.ui \
    mainwindow.ui \
    scanwidget.ui \
    selfcheckwidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_CXXFLAGS += -execution-charset:utf-8
