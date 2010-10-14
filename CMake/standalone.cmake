#############################################
# REQUIRE ITK 3.20 OR LATER                 #
#############################################
FIND_PACKAGE(ITK 3.20 REQUIRED)
INCLUDE(${ITK_USE_FILE})

#############################################
# REQUIRE VTK                               #
#############################################
FIND_PACKAGE(VTK 5.6 REQUIRED)
INCLUDE (${VTK_USE_FILE})

#############################################
# REQUIRE FLTK                              #
#############################################
INCLUDE(${SNAP_SOURCE_DIR}/CMake/find_fltk_13.cmake)

# Look for OpenGL.
FIND_PACKAGE(OpenGL REQUIRED)

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

# Link libraries from the parent CMAKE file
LINK_LIBRARIES(ITKAlgorithms ITKCommon ITKBasicFilters)


