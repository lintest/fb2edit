########################################################
#  
#  This is a CMake configuration file.
#  To use it you need CMake which can be 
#  downloaded from here: 
#    http://www.cmake.org/cmake/resources/software.html
#
#########################################################

cmake_minimum_required(VERSION 2.6.0)

project(fb2edit)
set(PACKAGE_VERSION "0.0.2")
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VENDOR "LinTest")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "mail@lintest.ru")

message( STATUS "PACKAGE_NAME = ${PACKAGE_STRING}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/3rdparty/cmake)

find_package( Qt4 4.7.0 COMPONENTS QtCore QtGui QtMain QtNetwork QtWebkit QtXml QtXmlPatterns REQUIRED )
find_package( QScintilla )
find_package( LibXML2 )

file( GLOB FB2_HEAD source/*.hpp     )
file( GLOB FB2_SRCS source/*.cpp     )
file( GLOB FB2_UIS  source/*.ui      )
file( GLOB FB2_TSS  source/ts/*.ts   )

set(FB2_DEFINITIONS "PACKAGE_NAME=\"${PACKAGE_NAME}\"; PACKAGE_VENDOR=\"${PACKAGE_VENDOR}\"; PACKAGE_VERSION=\"${PACKAGE_VERSION}\"")
set_source_files_properties( source/fb2app.cpp PROPERTIES COMPILE_DEFINITIONS "${FB2_DEFINITIONS}")

set(FB2_RES
    3rdparty/gnome/gnome.qrc
    source/js/javascript.qrc
    source/res/fb2edit.qrc
)

set(CMAKE_IN_SOURCE_BUILD TRUE)

add_definitions(-Wall -g)

qt4_wrap_ui(UI_HEADERS ${FB2_UIS})
qt4_wrap_cpp(MOC_SRCS ${FB2_HEAD})
qt4_add_resources(RCC_SRCS ${FB2_RES})
qt4_add_translation(QMS_FILES ${FB2_TSS})

add_executable(fb2edit ${FB2_SRCS} ${FB2_HEAD} ${UI_HEADERS} ${MOC_SRCS} ${RCC_SRCS} ${QMS_FILES})

include(${QT_USE_FILE})
include_directories(${CMAKE_BINARY_DIR})
target_link_libraries(fb2edit ${QT_LIBRARIES})
add_definitions(${QT_DEFINITIONS})

if (QSCINTILLA_FOUND) 
    include_directories(${QSCINTILLA_INCLUDE_DIR})
    target_link_libraries(fb2edit ${QSCINTILLA_LIBRARIES})
endif (QSCINTILLA_FOUND) 

if (LIBXML2_FOUND) 
    include_directories(${LIBXML2_INCLUDE_DIRS})
    target_link_libraries(fb2edit ${LIBXML2_LIBRARIES})
    add_definitions(${LIBXML2_DEFINITIONS})
    add_definitions(-DFB2_USE_LIBXML2)
endif (LIBXML2_FOUND) 

set(LINUX_DESKTOP_FILE desktop/fb2edit.desktop)
set(LINUX_DESKTOP_ICON desktop/fb2edit048.png)
    
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
    install( FILES ${LINUX_DESKTOP_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications )
    install( FILES ${LINUX_DESKTOP_ICON} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps RENAME fb2edit.png )
    install( FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/fb2edit/translations )
endif()

