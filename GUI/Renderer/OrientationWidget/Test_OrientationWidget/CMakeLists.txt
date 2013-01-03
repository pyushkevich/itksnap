CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(OrientationWidgetGUI)

FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

# Find VTK.
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(DESIRED_QT_VERSION ${VTK_DESIRED_QT_VERSION} CACHE FILEPATH "")
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
IF(QT_USE_FILE)
  INCLUDE(${QT_USE_FILE})
ELSE(QT_USE_FILE)
  SET(QT_LIBRARIES   ${QT_QT_LIBRARY})
ENDIF(QT_USE_FILE)

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

# Look for OpenGL.
FIND_PACKAGE(OpenGL REQUIRED)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
  ${CONFIGURED_SOURCE_DIRECTORY}
  ${OPENGL_INCLUDE_PATH}
  ${QT_QTSCRIPT_INCLUDE_DIR}
  ${QT_QTSCRIPTTOOLS_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/../Reorient
  ${CMAKE_SOURCE_DIR}/../../../Qt/View/
  ${CMAKE_SOURCE_DIR}/../../../../Common
)

QT4_WRAP_UI(UI_SRCS OrientationWidgetGUI.ui)
QT4_WRAP_CPP(MOC_SRCS OrientationWidgetGUI.h)
SET(SRCS ${SRCS} ${MOC_SRCS}
  main.cxx
  OrientationWidget.h OrientationWidget.cxx
  OrientationWidgetGUI.h OrientationWidgetGUI.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/AxesWidget.h ${CMAKE_SOURCE_DIR}/../Reorient/AxesWidget.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/AbstractScannerHelper.h ${CMAKE_SOURCE_DIR}/../Reorient/AbstractScannerHelper.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/PolyDataAlgorithm2ActorPipe.h ${CMAKE_SOURCE_DIR}/../Reorient/PolyDataAlgorithm2ActorPipe.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/ScannedHuman.h ${CMAKE_SOURCE_DIR}/../Reorient/ScannedHuman.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/ScanningROI.h ${CMAKE_SOURCE_DIR}/../Reorient/ScanningROI.cxx
  ${CMAKE_SOURCE_DIR}/../Reorient/ReorientProps.h ${CMAKE_SOURCE_DIR}/../Reorient/ReorientProps.cxx
  ${CMAKE_SOURCE_DIR}/../../../Qt/View/QtAbstractOpenGLBox.h ${CMAKE_SOURCE_DIR}/../../../Qt/View/QtAbstractOpenGLBox.cxx
  ${CMAKE_SOURCE_DIR}/../../../Qt/View/QtSimpleOpenGLBox.h ${CMAKE_SOURCE_DIR}/../../../Qt/View/QtSimpleOpenGLBox.cxx
  ${CMAKE_SOURCE_DIR}/../../../../Common/SNAPCommon.h
  #${CMAKE_SOURCE_DIR}/../../../Common/SNAPEvents.h
)

ADD_EXECUTABLE(OrientationWidgetGUI ${SRCS} ${UI_SRCS})

TARGET_LINK_LIBRARIES( OrientationWidgetGUI
  QVTK
  ${QT_LIBRARIES}
  ${QT_QTSCRIPT_LIBRARY}
  ${QT_QTSCRIPTTOOLS_LIBRARY}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
  vtkVolumeRendering
)




