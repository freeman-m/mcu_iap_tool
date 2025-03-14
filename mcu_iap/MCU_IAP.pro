#-------------------------------------------------
#
# Project created by QtCreator 2025-03-06T17:38:45
#
#-------------------------------------------------

QT       += core gui serialport


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MCU_IAP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serialporthandler.cpp \
    dataprotocol.cpp

HEADERS  += mainwindow.h \
    serialporthandler.h \
    dataprotocol.h

FORMS    += mainwindow.ui
