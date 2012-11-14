#-------------------------------------------------
#
# Project created by QtCreator 2012-11-05T20:06:26
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = 未命名
TEMPLATE = app


SOURCES += \
    main.cpp \
    ckinectreader.cpp \
    cskeletonitem.cpp \
    copenni.cpp

HEADERS  +=

FORMS    +=

INCLUDEPATH += /usr/include/ni /usr/include/nite
LIBS += /usr/lib/libXnVNite_1_5_2.so /usr/lib/libOpenNI.so
