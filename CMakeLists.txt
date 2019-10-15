########################################################
#  
#  This is a CMake configuration file.
#  To use it you need CMake which can be 
#  downloaded from here: 
#    http://www.cmake.org/cmake/resources/software.html
#
#########################################################

cmake_minimum_required(VERSION 3.1.0)

project(fb2edit)
set(PACKAGE_VERSION "0.1.0")
set(PACKAGE_VENDOR "LinTest")
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_STRING "${PROJECT_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "lav@etersoft.ru")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()
    
message( STATUS "PACKAGE_NAME = ${PACKAGE_STRING}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/cmake)

#find_program( QT_QMAKE_EXECUTABLE NAMES qmake5 qmake-qt5 qmake-mac )
find_package( Qt5 5.5.0 COMPONENTS Core Gui Network Widgets WebKitWidgets Xml XmlPatterns LinguistTools REQUIRED )
find_package( LibXML2 )

file( GLOB FB2_HEAD source/*.hpp     )
file( GLOB FB2_SRCS source/*.cpp     )
file( GLOB FB2_UIS  source/*.ui      )
file( GLOB FB2_TSS  source/ts/*.ts   )

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GTK_PKG_FLAGS}")

if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
    exec_program("git --git-dir=${CMAKE_SOURCE_DIR}/.git log --format=%ai//%h%d -1" OUTPUT_VARIABLE COMMIT_INFO)
    message( STATUS "COMMIT_INFO = ${COMMIT_INFO}")
endif(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

set(FB2_DEFINITIONS "PACKAGE_NAME=\"${PACKAGE_NAME}\"; PACKAGE_VENDOR=\"${PACKAGE_VENDOR}\"; PACKAGE_VERSION=\"${PACKAGE_VERSION}\"; COMMIT_INFO=\"${COMMIT_INFO}\"")
set_source_files_properties( source/fb2app.cpp PROPERTIES COMPILE_DEFINITIONS "${FB2_DEFINITIONS}")

set(FB2_RES
    3rdparty/fb2/fb2.qrc
    3rdparty/gnome/gnome.qrc
    source/js/javascript.qrc
    source/res/fb2edit.qrc
)

set(CMAKE_IN_SOURCE_BUILD TRUE)

add_definitions(-Wall -g)

qt5_wrap_ui(UI_HEADERS ${FB2_UIS})
qt5_wrap_cpp(MOC_SRCS ${FB2_HEAD})
qt5_add_resources(RCC_SRCS ${FB2_RES})
qt5_add_translation(QMS_FILES ${FB2_TSS})

message( STATUS "PACKAGE_NAME_ = ${CMAKE_PREFIX_PATH}")

add_executable(fb2edit ${FB2_SRCS} ${FB2_HEAD} ${UI_HEADERS} ${MOC_SRCS} ${RCC_SRCS} ${QMS_FILES})

#include(${QT_USE_FILE})
#include_directories(${QT_INCLUDES})
include_directories(${CMAKE_BINARY_DIR})
target_link_libraries(fb2edit PUBLIC
    Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets Qt5::WebKitWidgets Qt5::Xml Qt5::XmlPatterns
    ${LIBXML2_LIBRARIES}
    )
#add_definitions(${QT_DEFINITIONS})

include_directories(${LIBXML2_INCLUDE_DIRS})
if (LIBXML2_FOUND) 
    add_definitions(${LIBXML2_DEFINITIONS})
    add_definitions(-DFB2_USE_LIBXML2)
endif (LIBXML2_FOUND) 
   
#############################################################################
# You can change the install location by 
# running cmake like this:
#
#   cmake -DCMAKE_INSTALL_PREFIX=/new/install/prefix
#
# By default, the prefix is "/usr/local"
# 
if( UNIX AND NOT APPLE )
    install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin )
    install( FILES desktop/fb2edit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications )
    install( FILES desktop/fb2edit048.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME fb2edit.png )
    install( FILES desktop/fb2edit128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME fb2edit.png )
    install( FILES desktop/fb2edit256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME fb2edit.png )
    install( FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/fb2edit/translations )
    install( CODE "FILE(MAKE_DIRECTORY WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/pixmaps/)" )
    install( CODE "EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E create_symlink ../icons/hicolor/48x48/apps/fb2edit.png fb2edit.png WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/pixmaps)" )
endif()
