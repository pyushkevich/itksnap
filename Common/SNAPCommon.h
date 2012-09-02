/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPCommon.h,v $
  Language:  C++
  Date:      $Date: 2010/07/01 21:40:24 $
  Version:   $Revision: 1.13 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __SNAPCommon_h_
#define __SNAPCommon_h_

/** This is an include file that should be inserted into all SNAP files */

// Some standard library things are used so commonly that there is no need
// to have them included every time
#include <assert.h>
#include <iostream>
#include <ctime>

// Specify a standard stream for verbose messages
extern std::ostream &verbose;

// Specify a standard stream for error messages
extern std::ostream &snaperr;

// From ITK we use SmartPointer so many times that it is necessary
#include <itkSmartPointer.h>

// Include the VectorNx defintions
#include "IRISVectorTypes.h"

// RGB Pixel support
#include <itkRGBPixel.h>

/** Definitions for the string streams, for compatibility */
typedef std::ostringstream IRISOStringStream;
typedef std::istringstream IRISIStringStream;

// Non-cvs version
extern const char SNAPSoftVersion[];

// Just the number part of the SNAP version
extern unsigned int SNAPVersionMajor;
extern unsigned int SNAPVersionMinor;
extern unsigned int SNAPVersionPatch;
extern const char SNAPVersionQualifier[];

// Hardware architecture for this build
extern const char SNAPArch[];

// A string that appears in the user interface
extern const char SNAPUISoftVersion[];

// Release date of the current version
extern const char SNAPCurrentVersionReleaseDate[];

// Release date of the latest version whose user preference files are
// incompatible with the current version and will be erased
extern const char SNAPLastIncompatibleReleaseDate[];

// Voxel types 
// CAREFUL: do not redefine this to INT without disabling the usage of
// UnaryFunctorCache in the GreyImageWrapper type.  Greyscale instensities
// 0 to MAXGREYVAL are used in a cache table, which would be too big with int
typedef unsigned short LabelType;
typedef short GreyType;
extern const GreyType MAXGREYVAL;
extern const GreyType MINGREYVAL;
typedef itk::RGBPixel<unsigned char> RGBType;

#define MAX_COLOR_LABELS 0xffff

/** 
 A structure used to map intensity from gray to 'native' intensity for
 images that are not of short datatype
 */
struct GreyTypeToNativeFunctor
{
  double scale;
  double shift;
  double operator() (GreyType g) const
    { return g * scale + shift; }
  GreyTypeToNativeFunctor() : scale(1.0), shift(0.0) {}
  GreyTypeToNativeFunctor(double a, double b) : scale(a), shift(b) {}
};

/** 
 A structure used to map 'native' intensity to gray intensity for
 images that are not of short datatype
 */
struct NativeToGreyTypeFunctor
{
  double scale;
  double shift;
  GreyType operator() (double g) const
    { return GreyType(g * scale + shift); }
  NativeToGreyTypeFunctor() : scale(1.0), shift(0.0) {}
  NativeToGreyTypeFunctor(double a, double b) : scale(a), shift(b) {}
  NativeToGreyTypeFunctor(GreyTypeToNativeFunctor g2n) : scale(1.0/g2n.scale), shift(-g2n.shift/g2n.scale) {}
};

/************************************************************************/
/* PY: Some macros because I am tired of typing get/set                 */
/************************************************************************/

/**
 * Set macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisSetMacro(name,type) \
    virtual void Set##name (type _arg) \
{ \
    this->m_##name = _arg; \
} 

/**
 * Get macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisGetMacro(name,type) \
    virtual type Get##name () const { \
    return this->m_##name; \
} 

/**
 * Set macro for strings
 */
#define irisSetStringMacro(name) \
    virtual void Set##name (const char *_arg) \
{ \
    this->m_##name = _arg; \
} 

/**
 * Get macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisGetStringMacro(name) \
    virtual const char *Get##name () const { \
    return this->m_##name.c_str(); \
} 

/**
 * A get macro for boolean variables, IsXXX instead of GetXXX
 */
#define irisIsMacro(name) \
    virtual bool Is##name () const { \
    return this->m_##name; \
} 

/** 
 * A rip off from the ITK not used macro to eliminate 'parameter not used'
 * warnings 
 */
#define irisNotUsed(x)

#endif // __SNAPCommonIO_h_
