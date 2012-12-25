TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += \ 
    ../src/filter.cpp \
    ../src/dataacquire.cpp \
    ../src/acquire.cpp \
    ../test/test.cpp \
    ../src/adc.cpp \
    ../src/normalizeconfig.cpp \
    ../src/normalizer.cpp \
    ../../utils/byconfig/src/ByDBConfig.cpp \
    ../../utils/byconfig/src/ByConfigMgr.cpp \
    ../../utils/byconfig/src/ByConfig.cpp \
    ../../utils/byconfig/src/ByIniConfig.cpp \
    ../../utils/byconfig/src/CppSQLite3.cpp \
    ../src/virtualadc.cpp

UTILS_DIR += ../../utils
INCLUDEPATH += ../include
INCLUDEPATH += ../../common/include
INCLUDEPATH += $$UTILS_DIR/serialport/include
INCLUDEPATH += $$UTILS_DIR/byconfig/include
LIBS+=-lbySerialPort -lrt -lPocoFoundation -lsqlite3
OBJECTS_DIR += ./obj

linux-arm-g++ {
    message(g++ = linux-arm-g++ compile)
    LIBS += -L$$UTILS_DIR/serialport/linux/lib/arm
}

linux-g++ {
    message(g++ = linux-g++)
    LIBS += -L$$UTILS_DIR/serialport/linux/lib/x86
}
QMAKE_CXX += -g
HEADERS += \
    ../src/filter.h \
    ../src/dataacquire.h \
    ../include/acquire.h \
    ../src/adc.h \
    ../src/normalizeconfig.h \
    ../../common/include/css100.h \
    ../src/virtualadc.h