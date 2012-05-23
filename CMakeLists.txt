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

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package( Qt4 4.7.0 COMPONENTS QtCore QtGui QtMain QtNetwork QtWebkit QtXml QtXmlPatterns REQUIRED )
find_package( QScintilla REQUIRED )
find_package( LibXML2 REQUIRED )

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

file( GLOB FB2_HEAD source/*.h       )
file( GLOB FB2_SRCS source/*.cpp     )
file( GLOB FB2_UIS  source/*.ui      )
file( GLOB FB2_RES  source/res/*.qrc )
file( GLOB FB2_TS   source/ts/*.ts   )

set(CMAKE_IN_SOURCE_BUILD TRUE)

add_definitions(-Wall -g)

include(${QT_USE_FILE})
qt4_wrap_ui(UI_HEADERS ${FB2_UIS})
qt4_wrap_cpp(MOC_SRCS ${FB2_HEAD})
qt4_add_resources(RCC_SRCS ${FB2_RES})
qt4_create_translation(QM_FILES ${FB2_TS})
#QT4_CREATE_TRANSLATION(LangSrcs ${Lang_files})
include_directories(${CMAKE_BINARY_DIR} ${QSCINTILLA_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIRS})
add_executable(fb2edit ${FB2_SRCS} ${FB2_HEAD} ${MOC_SRCS} ${RCC_SRCS} ${QM_FILES})
target_link_libraries(fb2edit ${QT_LIBRARIES} ${QSCINTILLA_LIBRARIES} ${LIBXML2_LIBRARIES})

set(LINUX_DESKTOP_FILE desktop/fb2edit.desktop)
set(LINUX_DESKTOP_ICON desktop/fb2edit48.png)
    
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

