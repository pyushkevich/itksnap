INCLUDE_DIRECTORIES(${SNAP_SOURCE_DIR}/Utilities/FLTK/Fl_Table/FL)
INCLUDE_DIRECTORIES(${SNAP_SOURCE_DIR}/Utilities/FLTK/Fl_Table)
ADD_LIBRARY(fltk_table Fl_Table.cxx Fl_Table_Row.cxx)
