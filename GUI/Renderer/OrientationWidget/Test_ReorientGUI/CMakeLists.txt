CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(ReorientGUI)

# Find VTK.
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

# use what QVTK built with
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_Q MAKE_EXECUTABLE} CACHE FILEPATH "")
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(DESIRED_QT_VERSION ${VTK_DESIRED_QT_VERSION} CACHE FILEPATH "")
FIND_PACKAGE(Qt)
IF(QT_USE_FILE)
  INCLUDE(${QT_USE_FILE})
ELSE(QT_USE_FILE)
  SET(QT_LIBRARIES   ${QT_QT_LIBRARY})
ENDIF(QT_USE_FILE)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
        ${QT_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

QT4_WRAP_UI(UI_SRCS ReorientGUI.ui)
QT4_WRAP_CPP(MOC_SRCS ReorientGUI.h)
SET(SRCS ${SRCS} ${MOC_SRCS}
    main.cpp
	AxesWidget.h AxesWidget.cpp
	AbstractScannerHelper.h AbstractScannerHelper.cpp
	Source2ActorPipe.h Source2ActorPipe.cpp
	ScannedHuman.h ScannedHuman.cpp
	ScanningROI.h ScanningROI.cpp
	Reorient.cpp Reorient.h
	ReorientGUI.cpp ReorientGUI.h
	)

ADD_EXECUTABLE(ReorientGUI ${SRCS} ${UI_SRCS})

TARGET_LINK_LIBRARIES( ReorientGUI
  QVTK
  ${QT_LIBRARIES}
  vtkRendering
  vtkGraphics
  vtkIO
  vtkCommon
  vtkVolumeRendering
)




