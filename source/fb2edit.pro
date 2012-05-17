HEADERS = \
    fb2app.h \
    fb2main.h \
    fb2read.h \
    fb2tree.h \
    fb2save.h \
    fb2view.h \
    fb2xml.h \
    fb2xml2.h

SOURCES = \
    fb2app.cpp \
    fb2main.cpp \
    fb2read.cpp \
    fb2tree.cpp \
    fb2save.cpp \
    fb2view.cpp \
    fb2xml.cpp \
    fb2xml2.cpp

RESOURCES = \
    res/fb2edit.qrc

TARGET = fb2edit

TRANSLATIONS = ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

LIBS += -lqscintilla2

OTHER_FILES += res/style.css

if (win32) {

    INCLUDEPATH += ../libxml2/include
    LIBS += -L../libxml2/lib -llibxml2

    INCLUDEPATH += ../iconv/include
    LIBS += -L../iconv/lib -liconv

} else {

    INCLUDEPATH += /usr/include/libxml2
    LIBS += -lxml2

}
