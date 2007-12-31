#!/bin/bash

# *************************************************************
# SNAP UNIX/LINUX INSTALL SCRIPT - READ CAREFULLY
# *************************************************************
# This script will download and build SNAP and all the
# supporting software. In the ideal world, all you have to
# do is wait for an hour or so for this script to do all the
# work. 
#
# However, you must have gmake, gcc, cvs and svn (subversion)
# installed on your system. You also need to have an opengl
# driver. If SNAP behaves erratically and crashes, it may be
# due to a bad GL driver.
#
# The script will require almost 500MB of space. You can delete
# all files except those in the 'install' directory. However, 
# if you keep the files, you will be able to update SNAP quickly
# by running the script again.
#
# This script downloads ITK, VTK, FLTK and CMAKE. If you have 
# these programs already, you can modify the script to include
# them. Beware that the build flags must match those used in 
# this script.
#
# Please report build problems to itksnap-users Google Group
#
# Good Luck!!!
# Paul Y.
# *************************************************************

# Uncomment the line below to debug
### set -x -e

# -------------------------------------------------------------
# Options that the user can supply
# -------------------------------------------------------------

# The directory where the build will take place
basedir="`pwd`/itksnap_build"

# The installation directory, defaults to $basedir/install; 
# SNAP executable goes here (in bin/itksnap)
instdir=$basedir/install

# The location of the build log file
logfile=$basedir/buildlog

# Release versions for the supporting software. Change to HEAD to get
# the most current version of the code
CMAKE_REL="CMake-2-4-7"
ITK_REL="ITK-3-2"
VTK_REL="VTK-5-0-3"
SNAP_REL="HEAD"

# -------------------------------------------------------------
# Check for necessary software
# -------------------------------------------------------------
for prog in "cvs" "svn" "g++" "gcc" "autoconf" "make"
do
  $prog --version > /dev/null
  if [ $? -ne 0 ]
  then
    echo "Can not execute $prog on your system"
    echo "Please ask the administrator to install $prog and to "
    echo "make sure that $prog is in your path"
    exit
  fi
done

# -------------------------------------------------------------
# Initialization
# -------------------------------------------------------------
echo "PLEASE READ THE COMMENTS AT THE TOP OF THIS SCRIPT!!!"
echo "Preparing to build itk-snap. This will take 1-2 hours and 500MB!"
echo "Log messages are placed into $logfile"

# Create the directory tree for the build
for dir in "$basedir/itksnap/bingcc" \
  "$basedir/itk/bingcc" \
  "$basedir/vtk/bingcc" \
  "$basedir/fltk/bingcc" \
  "$basedir/cmake/install" \
  $instdir
do
  mkdir -p $dir
  if [ $? -ne 0 ]; then exit; fi
done

# Create a file for CMAKE password entries
echo "/1 :pserver:anonymous@www.cmake.org:2401/cvsroot/CMake Ah%y]d" > $basedir/.cvspass
echo "/1 :pserver:anonymous@public.kitware.com:2401/cvsroot/VTK A<,]" >> $basedir/.cvspass
echo "/1 :pserver:anonymous@www.itk.org:2401/cvsroot/Insight A?=Z?Ic," >> $basedir/.cvspass
export CVS_PASSFILE=$basedir/.cvspass

# Set some variables
cmakebin=$basedir/cmake/install/bin/cmake

# -------------------------------------------------------------
# Check out and build cmake
# -------------------------------------------------------------
function get_cmake {
  cd $basedir/cmake

  # Use CVS to check out the right version
  echo "Checking out CMake (Release ${CMAKE_REL}) from CVS"
  cvs -qd :pserver:anonymous@www.cmake.org:/cvsroot/CMake co -r ${CMAKE_REL} cmake > $logfile

  # Build cmake
  echo "Building CMake"
  cd $basedir/cmake/CMake
  ./configure --prefix=$basedir/cmake/install >> $logfile
  make >> $logfile
  make install >> $logfile

  # Test to see if everything is ok
  if [ ! -x $cmakebin ]
  then
    echo "Failed to create CMake executable"
    exit -1
  fi
}

# -------------------------------------------------------------
# Check out and build FLTK
# -------------------------------------------------------------
function get_fltk {
  cd $basedir/fltk

  # Use SVN to check out fltk
  echo "Checking out FLTK 1.1.7 from SVN"
  svn co http://svn.easysw.com/public/fltk/fltk/tags/release-1.1.7/ fltk-1.1.7 >> $logfile

  # Run CCMAKE in the FLTK bin directory
  echo "Building FLTK"
  cd $basedir/fltk/bingcc
  $cmakebin \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_TESTING:BOOL=ON \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_CXX_FLAGS_RELEASE:STRING="-O3 -DNDEBUG" \
    $basedir/fltk/fltk-1.1.7 >> $logfile
  make >> $logfile

  # Make sure that we have the necessary files
  for file in "bin/fluid" "bin/libfltk.a"
  do
    if [ ! -e $basedir/fltk/bingcc/$file ]
    then
      echo "Failed to create FLTK object $basedir/fltk/bingcc/$file"
      exit -1
    fi
  done 
} 

# -------------------------------------------------------------
# Check out and build VTK
# -------------------------------------------------------------
function get_vtk {
  cd $basedir/vtk

  # Use CVS to check out VTK
  echo "Checking out VTK (Release ${VTK_REL}) from CVS"
  cvs -qd :pserver:anonymous@public.kitware.com:/cvsroot/VTK co -r ${VTK_REL} VTK >> $logfile

  # Configure VTK using CMake
  echo "Building VTK"
  cd $basedir/vtk/bingcc
  $cmakebin \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_TESTING:BOOL=OFF \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DVTK_USE_HYBRID:BOOL=ON \
    -DVTK_USE_ANSI_STDLIB:BOOL=ON \
    -DVTK_USE_PARALLEL:BOOL=ON \
    -DVTK_USE_RENDERING:BOOL=ON \
    -DVTK_USE_PATENTED:BOOL=ON \
    -DCMAKE_CXX_FLAGS_RELEASE:STRING="-O3 -DNDEBUG" \
    $basedir/vtk/VTK >> $logfile
  make >> $logfile

  # Check whether the necessary libraries built
  for lib in "Common" "IO" "Graphics" "Imaging" "Filtering"
  do
    if [ ! -e $basedir/vtk/bingcc/bin/libvtk${lib}.a ]
    then
      echo "VTK library $basedir/vtk/bingcc/bin/libvtk${lib}.a failed to build!"
      exit -1
    fi
  done
}

# -------------------------------------------------------------
# Check out and build ITK
# -------------------------------------------------------------
function get_itk {
  cd $basedir/itk

  # Use CVS to check out VTK
  echo "Checking out ITK (Release ${ITK_REL}) from CVS"
  cvs -qd :pserver:anonymous@www.itk.org:/cvsroot/Insight co -r ${ITK_REL} Insight >> $logfile

  # Configure VTK using CMake
  echo "Building ITK"
  cd $basedir/itk/bingcc
  $cmakebin \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_TESTING:BOOL=OFF \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_CXX_FLAGS_RELEASE:STRING="-O3 -DNDEBUG" \
    $basedir/itk/Insight >> $logfile
  make >> $logfile

  # Check whether the necessary libraries built
  for lib in "Common" "IO" "BasicFilters" "Algorithms"
  do
    if [ ! -e $basedir/itk/bingcc/bin/libITK${lib}.a ]
    then
      echo "ITK library $basedir/itk/bingcc/bin/libITK${lib}.a failed to build!"
      exit -1
    fi
  done
}

# -------------------------------------------------------------
# Check out and build ITK-SNAP
# -------------------------------------------------------------
function get_itksnap {
  cd $basedir/itksnap

  # Use CVS to check out InsightApplications
  echo "Checking out ITK-SNAP (Release $SNAP_REL) from CVS"
  # cvs -qd :pserver:anonymous@www.itk.org:/cvsroot/Insight co -r $SNAP_REL InsightApplications >> $logfile
  cvs -qd :pserver:itk-snap.cvs.sourceforge.net/cvsroot/itk-snap co -r $SNAP_REL itksnap

  # Configure VTK using CMake
  echo "Building SNAP"
  cd $basedir/itksnap/bingcc
  $cmakebin \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_TESTING:BOOL=OFF \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_CXX_FLAGS_RELEASE:STRING="-O3 -DNDEBUG" \
    -DUSE_ITK:BOOL=ON \
    -DUSE_VTK:BOOL=ON \
    -DUSE_FLTK:BOOL=ON \
    -DITK_DIR:PATH="$basedir/itk/bingcc" \
    -DVTK_DIR:PATH="$basedir/vtk/bingcc" \
    -DFLTK_DIR:PATH="$basedir/fltk/bingcc" \
    -DCMAKE_INSTALL_PREFIX:PATH=$instdir \
    $basedir/itksnap/itksnap >> $logfile

  # Make only in the SNAP directory
  make >> $logfile
  make install >> $logfile

  if [ ! -f $instdir/bin/InsightSNAP ]
  then
    echo "SNAP failed to build in $instdir/bin/InsightSNAP"
    exit -1;
  fi
}

# -------------------------------------------------------------
# Perform the actual build tasks
# -------------------------------------------------------------
get_cmake
get_itk
get_fltk
get_vtk
get_itksnap

echo "SNAP executable is located in $instdir/bin/InsightSNAP!"
