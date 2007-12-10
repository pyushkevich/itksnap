/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPCommon.h,v $
  Language:  C++
  Date:      $Date: 2007/12/10 01:14:01 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
typedef itk::OStringStream IRISOStringStream;
typedef std::istringstream IRISIStringStream;

// Non-cvs version
extern const char SNAPSoftVersion[];

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

#define MAX_COLOR_LABELS 1024

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
