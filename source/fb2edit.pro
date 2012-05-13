HEADERS = \
    fb2app.h \
    fb2main.h \
    fb2read.h \
    fb2tree.h \
    fb2view.h \
    fb2xml.h

SOURCES = \
    fb2app.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2tree.cpp \
    fb2view.cpp

RESOURCES = \
    res/fb2edit.qrc

TARGET = fb2edit

TRANSLATIONS = res/ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit

LIBS += -lqscintilla2

OTHER_FILES += res/style.css
