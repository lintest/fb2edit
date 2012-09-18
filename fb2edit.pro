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
    source/fb2save.hpp \
    source/fb2dlgs.hpp \
    source/fb2html.h \
    source/fb2text.hpp \
    source/fb2utils.h

SOURCES = \
    source/fb2app.cpp \
    source/fb2code.cpp \
    source/fb2head.cpp \
    source/fb2main.cpp \
    source/fb2read.cpp \
    source/fb2save.cpp \
    source/fb2temp.cpp \
    source/fb2tree.cpp \
    source/fb2xml.cpp \
    source/fb2xml2.cpp \
    source/fb2utils.cpp \
    source/fb2dlgs.cpp \
    source/fb2html.cpp \
    source/fb2text.cpp

RESOURCES = \
    3rdparty/gnome/gnome.qrc \
    source/res/fb2edit.qrc \
    source/js/javascript.qrc \
    3rdparty/fb2/fb2.qrc

TARGET = fb2edit

TRANSLATIONS = source/ts/ru.ts

VERSION = 0.01.1

QT += xml
QT += webkit
QT += network

OTHER_FILES += \
    source/res/style.css \
    source/res/blank.fb2 \
    source/js/export.js \
    source/js/set_cursor.js \
    source/js/get_status.js \
    source/js/get_location.js \
    source/js/insert_title.js \
    CMakeLists.txt \
    source/js/new_section.js

if (unix) {

    DEFINES += FB2_USE_LIBXML2
    INCLUDEPATH += /usr/include/libxml2
    LIBS += -lxml2

}

FORMS += \
    source/fb2find.ui \
    source/fb2setup.ui
