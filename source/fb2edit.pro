HEADERS = \
    fb2app.h \
    fb2doc.h \
    fb2main.h \
    fb2read.h \
    fb2text.h \
    fb2tree.h

SOURCES = \
    fb2app.cpp \
    fb2doc.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2tree.cpp

RESOURCES = \
    fb2edit.qrc

TARGET = fb2edit

VERSION = 0.01.1

QT += xml

LIBS += -lqscintilla2
