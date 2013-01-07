
SRC_DIR = $(CURDIR)
BUILD_DIR = $(CURDIR)/../build

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(subst $(SRC_DIR),$(BUILD_DIR),$(patsubst %.cpp, %.o, $(SRCS)))
#INC = /Applications/MATLAB_R2012a.app/extern/include/

#$(info $(OBJS))

BIN = ../rf_snap

CC = g++
FORTRAN = gfortran
CCFLAGS = -fpic -O2 -funroll-loops -Wall #-march=native
FFLAGS = -O2 -fpic -m64

.PHONY : all

all:	$(BIN)

$(BIN):	rfsub $(OBJS) rf_snap.o
	$(CC) $(CCFLAGS) -o $(BIN) $(OBJS) $(BUILD_DIR)/rfsub.o #-lgfortran #-lm

$(BUILD_DIR)/%.o:	$(SRC_DIR)/%.cpp
	$(CC) $(CCFLAGS) -o $(subst $(SRC_DIR),$(BUILD_DIR),$@) -c $< #-I $(INC)

rf_snap.o:	$(SRC_DIR)/rf_snap.cpp
	$(CC) $(CCFLAGS) -o $(BUILD_DIR)/rf_snap.o -c $(SRC_DIR)/rf_snap.cpp
	
rfsub:	$(SRC_DIR)/rfsub.f
	if test -d $(BUILD_DIR);then true;else mkdir $(BUILD_DIR);fi
	$(FORTRAN)  $(FFLAGS) -c $(SRC_DIR)/rfsub.f -o $(BUILD_DIR)/rfsub.o
	
clean:	
	rm -rf $(BUILD_DIR)/* rf_snap
