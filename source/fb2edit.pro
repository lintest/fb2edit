HEADERS = \
    fb2app.h \
    fb2main.h \
    fb2read.h \
    fb2tree.h \
    fb2view.h

SOURCES = \
    fb2app.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2tree.cpp \
    fb2view.cpp

RESOURCES = \
    fb2edit.qrc

TARGET = fb2edit

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

LIBS += -lqscintilla2
