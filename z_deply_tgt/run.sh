#!/bin/sh

export QT_XCB_GL_INTEGRATION=none

export LD_LIBRARY_PATH=$PWD/lib
export QT_QPA_PLATFORM_PLUGIN_PATH=$PWD/plugins
./x-data-scanner
