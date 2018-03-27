#############################################
# REQUIRE ITK 3.20 OR LATER                 #
#############################################
FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

#############################################
# REQUIRE VTK                               #
#############################################
FIND_PACKAGE(VTK REQUIRED)
INCLUDE (${VTK_USE_FILE})

#############################################
# REQUIRE QT                                #
#############################################
IF(NOT SNAP_USE_QT4)

  FIND_PACKAGE(Qt5Widgets)
  FIND_PACKAGE(Qt5OpenGL)
  FIND_PACKAGE(Qt5Concurrent)
  FIND_PACKAGE(Qt5Qml)

  SET(SNAP_QT_INCLUDE_DIRS
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${Qt5Concurrent_INCLUDE_DIRS}
    ${Qt5Qml_INCLUDE_DIRS}
  )

  SET(SNAP_QT_LIBRARIES
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Concurrent
    Qt5::Qml
  )

  # Set vars for the QT binary and library directories
  GET_FILENAME_COMPONENT(QT_BINARY_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
  GET_FILENAME_COMPONENT(QT_LIBRARY_DIR "${Qt5Core_DIR}/../../" ABSOLUTE)

  # Set the QTVERSION var
  SET(QTVERSION ${Qt5Widgets_VERSION})

ELSE(NOT SNAP_USE_QT4)

  FIND_PACKAGE(Qt4 4.8 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
  INCLUDE(${QT_USE_FILE})
  ADD_DEFINITIONS(${QT_DEFINITIONS})

  SET(SNAP_QT_INCLUDE_DIRS
    ${QT_QTSCRIPT_INCLUDE_DIR}
    ${QT_QTSCRIPTTOOLS_INCLUDE_DIR}
  )

  SET(SNAP_QT_LIBRARIES
    ${QT_LIBRARIES}
    ${QT_QTMAIN_LIBRARY}
    ${QT_QTSCRIPT_LIBRARY}
    ${QT_QTSCRIPTTOOLS_LIBRARY}
  )

ENDIF(NOT SNAP_USE_QT4)

# Look for OpenGL.
FIND_PACKAGE(OpenGL REQUIRED)

# Look for GLKit on Apple
IF(APPLE)
  FIND_LIBRARY(MACOS_GLKIT_LIBRARY GLKit)
  MARK_AS_ADVANCED(MACOS_GLKIT_LIBRARY)
  SET(SNAP_OPENGL_LIBS ${OPENGL_LIBRARIES} ${MACOS_GLKIT_LIBRARY})
ELSE(APPLE)
  SET(SNAP_OPENGL_LIBS ${OPENGL_LIBRARIES} ${OPENGL_glu_LIBRARY})
ENDIF(APPLE)

# Look for CURL. It is now required part of ITK-SNAP
FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
ENDIF(CURL_FOUND)


