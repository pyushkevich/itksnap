# CMake project file for RandomForest

cmake_minimum_required (VERSION 2.6)
project (RandomForest)
enable_language (Fortran)

# make sure that the default is a RELEASE
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RELEASE CACHE STRING
      "Choose the type of build, options are: None Debug Release."
      FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# default installation
get_filename_component (default_prefix ".." ABSOLUTE)
set (CMAKE_INSTALL_PREFIX ${default_prefix} CACHE STRING
      "Choose the installation directory; by default it installs in the RandomForest directory." FORCE)

# FFLAGS depend on the compiler
get_filename_component (Fortran_COMPILER_NAME ${CMAKE_Fortran_COMPILER} NAME)

if (Fortran_COMPILER_NAME STREQUAL "gfortran")
  # gfortran
  #set (CMAKE_Fortran_FLAGS_RELEASE "-funroll-all-loops -fno-f2c -O3")
  #set (CMAKE_Fortran_FLAGS_DEBUG   "-fno-f2c -O0 -g")
  set (CMAKE_Fortran_FLAGS_RELEASE   "-O2 -fpic -m64")
  set (CMAKE_Fortran_FLAGS_RELEASE   "-g -fpic -m64")
elseif (Fortran_COMPILER_NAME STREQUAL "ifort")
  # ifort (untested)
  set (CMAKE_Fortran_FLAGS_RELEASE "-f77rtl -O3")
  set (CMAKE_Fortran_FLAGS_DEBUG   "-f77rtl -O0 -g")
elseif (Fortran_COMPILER_NAME STREQUAL "g77")
  # g77
  set (CMAKE_Fortran_FLAGS_RELEASE "-funroll-all-loops -fno-f2c -O3 -m32")
  set (CMAKE_Fortran_FLAGS_DEBUG   "-fno-f2c -O0 -g -m32")
else (Fortran_COMPILER_NAME STREQUAL "gfortran")
  message ("CMAKE_Fortran_COMPILER full path: " ${CMAKE_Fortran_COMPILER})
  message ("Fortran compiler: " ${Fortran_COMPILER_NAME})
  message ("No optimized Fortran compiler flags are known, we just try -O2...")
  set (CMAKE_Fortran_FLAGS_RELEASE "-O2")
  set (CMAKE_Fortran_FLAGS_DEBUG   "-O0 -g")
endif (Fortran_COMPILER_NAME STREQUAL "gfortran")

set(CMAKE_CXX_FLAGS "-fpic -O2 -funroll-loops -Wall" #-march=native
)

add_executable (rf_snap 
  rf_snap.cpp
  classRF.cpp
  classTree.cpp
  cokus.cpp
  rfutils.cpp
  qsort.c
  rf.h
  rfsub.f
)

# install executables and scripts
install (TARGETS ${EXECUTABLES} RUNTIME DESTINATION "bin")
install (PROGRAMS ${SCRIPTS} DESTINATION "bin") 

###########################################################




#.PHONY : all

#all:	$(BIN)

#$(BIN):	rfsub $(OBJS) rf_snap.o
#	$(CC) $(CCFLAGS) -o $(BIN) $(OBJS) $(BUILD_DIR)/rf_snap.o $(BUILD_DIR)/rfsub.o #-lgfortran #-lm

#$(BUILD_DIR)/%.o:	$(SRC_DIR)/%.cpp
#	$(CC) $(CCFLAGS) -o $(subst $(SRC_DIR),$(BUILD_DIR),$@) -c $< #-I $(INC)

#rf_snap.o:	rf_snap.cpp
#	$(CC) $(CCFLAGS) -o $(BUILD_DIR)/rf_snap.o -c rf_snap.cpp
	
#rfsub:	$(SRC_DIR)/rfsub.f
#	if test -d $(BUILD_DIR);then true;else mkdir $(BUILD_DIR);fi
#	$(FORTRAN)  $(FFLAGS) -c $(SRC_DIR)/rfsub.f -o $(BUILD_DIR)/rfsub.o
	
#clean:	
#	rm -rf $(BUILD_DIR)/* rf_snap
