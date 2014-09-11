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
FIND_PACKAGE(Qt5Widgets)
FIND_PACKAGE(Qt5OpenGL)
FIND_PACKAGE(Qt5Concurrent)
FIND_PACKAGE(Qt5Qml)

SET(SNAP_QT5_INCLUDE_DIRS
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5OpenGL_INCLUDE_DIRS}
  ${Qt5Concurrent_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS}
)

SET(SNAP_QT5_LIBRARIES
  Qt5::Widgets
  Qt5::OpenGL
  Qt5::Concurrent
  Qt5::Qml
)

# Set vars for the QT binary and library directories
GET_FILENAME_COMPONENT(QT_BINARY_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
GET_FILENAME_COMPONENT(QT_LIBRARY_DIR "${Qt5Core_DIR}/../../" ABSOLUTE)

# Look for OpenGL.
FIND_PACKAGE(OpenGL REQUIRED)

# Link libraries from the parent CMAKE file
#LINK_LIBRARIES(ITKAlgorithms ITKCommon ITKBasicFilters)


