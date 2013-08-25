#############################################
# REQUIRE ITK                               #
#############################################
FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

#############################################
# REQUIRE VTK                               #
#############################################
FIND_PACKAGE(VTK REQUIRED)
INCLUDE (${VTK_USE_FILE})

#############################################
# REQUIRE FLTK                              #
#############################################
INCLUDE(${SNAP_SOURCE_DIR}/CMake/find_fltk_13.cmake)

# Allow FLTK 1.1 for older systems. This is an optional flag
OPTION(SNAP_USE_FLTK_1_1 OFF "Build with older FLTK 1.1")
MARK_AS_ADVANCED(SNAP_USE_FLTK_1_1)
IF(SNAP_USE_FLTK_1_1)
  SUBDIRS(Utilities/FLTK/Fl_Table)
  SUBDIRS(Utilities/FLTK/Fl_Native_File_Chooser)
  SET(FLTK_LIBRARIES fltk_table fltk_native_file_chooser ${FLTK_LIBRARIES})
  SET(FLTK_INCLUDE_PATH ${FLTK_INCLUDE_PATH} 
    ${SNAP_SOURCE_DIR}/Utilities/FLTK/Fl_Table
    ${SNAP_SOURCE_DIR}/Utilities/FLTK/Fl_Native_File_Chooser)
ENDIF(SNAP_USE_FLTK_1_1)

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

