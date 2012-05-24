HEADERS = \
    source/fb2app.h \
    source/fb2code.h \
    source/fb2head.h \
    source/fb2main.h \
    source/fb2read.h \
    source/fb2tree.h \
    source/fb2save.h \
    source/fb2view.h \
    source/fb2xml.h \
    source/fb2xml2.h

SOURCES = \
    source/fb2app.cpp \
    source/fb2code.cpp \
    source/fb2head.cpp \
    source/fb2main.cpp \
    source/fb2read.cpp \
    source/fb2tree.cpp \
    source/fb2save.cpp \
    source/fb2view.cpp \
    source/fb2xml.cpp \
    source/fb2xml2.cpp

RESOURCES = \
    3rdparty/gnome/gnome.qrc \
    source/res/fb2edit.qrc

TARGET = fb2edit

TRANSLATIONS = source/ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

LIBS += -lqscintilla2

OTHER_FILES += \
    source/res/style.css \
    source/res/blank.fb2

if (win32) {

    INCLUDEPATH += ../libxml2/include
    INCLUDEPATH += ../iconv/include

    LIBS += -L../libxml2/lib -llibxml2
    LIBS += -L../iconv/lib -liconv
    LIBS += -L../zlib/lib -lzlib

} else {

    INCLUDEPATH += /usr/include/libxml2
    LIBS += -lxml2

}
