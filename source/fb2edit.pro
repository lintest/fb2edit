HEADERS = \
    fb2app.h \
    fb2main.h \
    fb2read.h \
    fb2tree.h \
    fb2view.h \
    fb2xml.h \
    fb2save.h

SOURCES = \
    fb2app.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2tree.cpp \
    fb2view.cpp \
    fb2xml.cpp \
    fb2save.cpp

RESOURCES = \
    res/fb2edit.qrc

TARGET = fb2edit

TRANSLATIONS = res/ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

LIBS += -lqscintilla2

OTHER_FILES += res/style.css
