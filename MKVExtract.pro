QT += core
QT -= gui

TARGET = MKVExtract
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

TEMPLATE = app

SOURCES += main.cpp \
    unrar.cpp \
    helper.cpp

HEADERS += \
    unrar.h \
    helper.h

