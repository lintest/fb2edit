HEADERS = \
    source/fb2html.h \
    source/fb2app.hpp \
    source/fb2code.hpp \
    source/fb2dlgs.hpp \
    source/fb2dock.hpp \
    source/fb2head.hpp \
    source/fb2imgs.hpp \
    source/fb2list.hpp \
    source/fb2main.hpp \
    source/fb2note.hpp \
    source/fb2page.hpp \
    source/fb2read.hpp \
    source/fb2tree.hpp \
    source/fb2save.hpp \
    source/fb2text.hpp \
    source/fb2utils.h \
    source/fb2xml.hpp \
    source/fb2mode.h \
    source/fb2xml2.h \
    source/fb2logs.hpp

SOURCES = \
    source/fb2app.cpp \
    source/fb2code.cpp \
    source/fb2dlgs.cpp \
    source/fb2dock.cpp \
    source/fb2head.cpp \
    source/fb2html.cpp \
    source/fb2imgs.cpp \
    source/fb2list.cpp \
    source/fb2main.cpp \
    source/fb2note.cpp \
    source/fb2page.cpp \
    source/fb2read.cpp \
    source/fb2save.cpp \
    source/fb2tree.cpp \
    source/fb2xml.cpp \
    source/fb2xml2.cpp \
    source/fb2text.cpp \
    source/fb2utils.cpp \
    source/fb2mode.cpp \
    source/fb2logs.cpp

RESOURCES = \
    3rdparty/gnome/gnome.qrc \
    source/res/fb2edit.qrc \
    source/js/javascript.qrc \
    3rdparty/fb2/fb2.qrc

TARGET = fb2edit

TRANSLATIONS = source/ts/ru.ts

QT += xml
QT += webkit
QT += network
QT += xmlpatterns

CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11
DEFINES += QT_USE_QSTRINGBUILDER

OTHER_FILES += \
    source/res/style.css \
    source/res/blank.fb2 \
    source/js/export.js \
    source/js/set_cursor.js \
    source/js/get_status.js \
    source/js/insert_title.js \
    CMakeLists.txt \
    source/js/new_section1.js \
    source/js/section_get.js \
    source/js/section_new.js \
    source/js/location.js \
    source/res/mainicon.rc \
    3rdparty/fb2/FictionBookLinks.xsd \
    3rdparty/fb2/FictionBookLang.xsd \
    3rdparty/fb2/FictionBookGenres.xsd \
    3rdparty/fb2/FictionBook2.1.xsd

if (unix) {

    DEFINES += FB2_USE_LIBXML2
    INCLUDEPATH += /usr/include/libxml2
    LIBS += -lxml2

}

FORMS += \
    source/fb2find.ui \
    source/fb2setup.ui

win32:RC_FILE = source/res/mainicon.rc
