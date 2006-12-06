FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR
    "Cannot build InsightApplications without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)

FIND_PACKAGE(FLTK)
IF(FLTK_FOUND)
  INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})
ENDIF(FLTK_FOUND)

FIND_PACKAGE(VTK)
IF (VTK_FOUND)
  INCLUDE (${VTK_USE_FILE})
ENDIF (VTK_FOUND)
 
# Look for OpenGL.
FIND_PACKAGE(OpenGL)
IF(OPENGL_INCLUDE_PATH)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_PATH})
ENDIF(OPENGL_INCLUDE_PATH)

# The fluid-generated fltk sources have many warnings.  This macro
# will disable warnings for the generated files on some compilers.
MACRO(ITK_DISABLE_FLTK_GENERATED_WARNINGS files)
  IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.6)
    IF(CMAKE_COMPILER_IS_GNUCXX)
      FOREACH(f ${files})
        STRING(REGEX REPLACE "\\.fl$" ".cxx" SRC "${f}")
        STRING(REGEX REPLACE ".*/([^/]*)$" "\\1" SRC "${SRC}")
        SET_SOURCE_FILES_PROPERTIES(${SRC} PROPERTIES COMPILE_FLAGS -w)
      ENDFOREACH(f)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
  ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.6)
ENDMACRO(ITK_DISABLE_FLTK_GENERATED_WARNINGS)

# Macro for adding resources to GUI application on the Mac
#
IF(APPLE AND NOT FLTK_USE_X)
    FIND_PROGRAM(ITK_APPLE_RESOURCE Rez /Developer/Tools)
    FIND_FILE(ITK_FLTK_RESOURCE mac.r /usr/local/include/FL)
    IF(NOT ITK_FLTK_RESOURCE)
      MESSAGE("Fltk resources not found, GUI application will not respond to mouse events")
    ENDIF(NOT ITK_FLTK_RESOURCE)

    MACRO(ADD_GUI_EXECUTABLE name sources)
      ADD_EXECUTABLE(${name} ${sources})
      INSTALL_TARGETS(/bin ${name})
      SET(EXEC_PATH ${EXECUTABLE_OUTPUT_PATH})
      IF(NOT EXEC_PATH)
        SET(EXEC_PATH ${CMAKE_CURRENT_BINARY_DIR})
      ENDIF(NOT EXEC_PATH)
        IF(ITK_APPLE_RESOURCE)
          ADD_CUSTOM_COMMAND(SOURCE ${name}
                             COMMAND ${ITK_APPLE_RESOURCE}
                             ARGS -t APPL ${ITK_FLTK_RESOURCE} -o
                             ${EXEC_PATH}/${name}
                             TARGET ${name})
        ENDIF(ITK_APPLE_RESOURCE)
    ENDMACRO(ADD_GUI_EXECUTABLE)
ELSE(APPLE AND NOT FLTK_USE_X)
  MACRO(ADD_GUI_EXECUTABLE name sources)
    ADD_EXECUTABLE(${name} ${sources})
    INSTALL_TARGETS(/bin ${name})
  ENDMACRO(ADD_GUI_EXECUTABLE)
ENDIF(APPLE AND NOT FLTK_USE_X)

# Flag that allows patented code in VTK to be used
OPTION(USE_VTK_PATENTED "Should patented VTK code be used?" OFF)
IF(USE_VTK_PATENTED)
  ADD_DEFINITIONS(-DUSE_VTK_PATENTED)
ENDIF(USE_VTK_PATENTED)
MARK_AS_ADVANCED(USE_VTK_PATENTED)

# Link libraries from the parent CMAKE file
LINK_LIBRARIES(ITKAlgorithms ITKCommon ITKBasicFilters)


