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
FIND_PACKAGE(Qt4 4.7 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

# Look for OpenGL.
FIND_PACKAGE(OpenGL REQUIRED)

# Link libraries from the parent CMAKE file
LINK_LIBRARIES(ITKAlgorithms ITKCommon ITKBasicFilters)


