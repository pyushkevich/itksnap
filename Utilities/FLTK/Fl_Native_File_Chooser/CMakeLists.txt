INCLUDE_DIRECTORIES(${SNAP_SOURCE_DIR}/Utilities/FLTK/Fl_Native_File_Chooser)
ADD_DEFINITIONS(-DFLTK1)	
ADD_LIBRARY(fltk_native_file_chooser Fl_Native_File_Chooser.cxx)