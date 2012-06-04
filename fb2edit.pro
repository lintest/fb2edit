HEADERS = \
    source/fb2xml.h \
    source/fb2xml2.h \
    source/fb2app.hpp \
    source/fb2code.hpp \
    source/fb2head.hpp \
    source/fb2main.hpp \
    source/fb2read.hpp \
    source/fb2temp.hpp \
    source/fb2tree.hpp \
    source/fb2view.hpp \
    source/fb2note.hpp \
    source/fb2utils.h \
    source/fb2save.hpp

SOURCES = \
    source/fb2app.cpp \
    source/fb2code.cpp \
    source/fb2head.cpp \
    source/fb2main.cpp \
    source/fb2read.cpp \
    source/fb2save.cpp \
    source/fb2temp.cpp \
    source/fb2tree.cpp \
    source/fb2view.cpp \
    source/fb2xml.cpp \
    source/fb2xml2.cpp \
    source/fb2note.cpp \
    source/fb2utils.cpp

RESOURCES = \
    3rdparty/gnome/gnome.qrc \
    source/res/fb2edit.qrc \
    source/js/javascript.qrc

TARGET = fb2edit

TRANSLATIONS = source/ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

LIBS += -lqscintilla2

OTHER_FILES += \
    source/res/style.css \
    source/res/blank.fb2 \
    source/js/export.js \
    source/js/set_cursor.js \
    source/js/get_status.js

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

FORMS += \
    source/fb2note.ui
