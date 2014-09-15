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

// Build date - shown to help debugging nightly builds
extern const char SNAPBuildInfo[];

// Voxel types 
// CAREFUL: do not redefine this to INT without disabling the usage of
// UnaryFunctorCache in the GreyImageWrapper type.  Greyscale instensities
// 0 to MAXGREYVAL are used in a cache table, which would be too big with int
typedef unsigned short LabelType;
typedef short GreyType;
extern const GreyType MAXGREYVAL;
extern const GreyType MINGREYVAL;
typedef itk::RGBPixel<unsigned char> RGBType;

/************************************************************************/
/* Enums that are common enough to declare them outside of a class      */
/************************************************************************/

// Coverage mode for draw-over operations
enum CoverageModeType
{
  PAINT_OVER_ALL = 0,
  PAINT_OVER_VISIBLE,
  PAINT_OVER_ONE
};

// Role played by an image layer.
enum LayerRole
{
  MAIN_ROLE = 0x0001,
  OVERLAY_ROLE = 0x0002,
  SNAP_ROLE = 0x0004,
  LABEL_ROLE = 0x0008,
  NO_ROLE = 0x0010,
  ALL_ROLES = 0xffffffff
};

// Cardinal directions in the anatomical space
enum AnatomicalDirection
{
  ANATOMY_AXIAL = 0,
  ANATOMY_SAGITTAL,
  ANATOMY_CORONAL,
  ANATOMY_NONSENSE
};



// An atomic data type to represent draw-over state
struct DrawOverFilter
{
  CoverageModeType CoverageMode;
  LabelType DrawOverLabel;

  bool operator == (const DrawOverFilter &cmp) const
  {
    return CoverageMode == cmp.CoverageMode
        && DrawOverLabel == cmp.DrawOverLabel;
  }

  bool operator != (const DrawOverFilter &cmp) const
  {
    return CoverageMode != cmp.CoverageMode
        || DrawOverLabel != cmp.DrawOverLabel;
  }

  DrawOverFilter()
    : CoverageMode(PAINT_OVER_ALL), DrawOverLabel(0) {}
  DrawOverFilter(CoverageModeType cm, LabelType label)
    : CoverageMode(cm), DrawOverLabel(label) {}
  DrawOverFilter(const DrawOverFilter &ref)
    : CoverageMode(ref.CoverageMode), DrawOverLabel(ref.DrawOverLabel) {}
};

#define MAX_COLOR_LABELS 0xffff
#define NUM_INITIAL_COLOR_LABELS 6


#define DEFAULT_HISTOGRAM_BINS 40

/**
  A debugging function to get the system time in ms. Actual definition is
  in SystemInterface.cxx
  */
long get_system_time_ms();


/**
  A child class around itk::SmartPointer. It's main purpose is to allow Qt
  Creator's intellisense to work with smart pointers. The current version
  (Qt 4.7.x) does not complete smart pointers because of typedefs used by
  the -> operator. In the future when this goes away (or does not matter)
  we can simply replace this class by "typedef itk::SmartPointer SmartPtr"
  */
template <class TObject> class SmartPtr : public itk::SmartPointer<TObject>
{
public:
  typedef SmartPtr<TObject> Self;
  typedef itk::SmartPointer<TObject> Superclass;

  TObject* operator -> () const
    { return Superclass::GetPointer(); }

  SmartPtr(const Superclass &p) : Superclass(p) {}
  SmartPtr(TObject *p) : Superclass(p) {}
  SmartPtr() : Superclass() {}

  Self &operator =(const Self &p)
  {
    Superclass::operator =(p);
    return *this;
  }
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
 * Get macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisStaticGetMacro(name,type) \
    static type Get##name () const { \
    return this->m_##name; \
}

/**
 * Set macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisVirtualSetMacro(name,type) \
    virtual void Set##name (type _arg) = 0;

/**
 * Get macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisVirtualGetMacro(name,type) \
    virtual type Get##name () const = 0;

/**
 * Combo get/set macro
 */
#define irisGetSetMacro(name,type) \
  virtual void Set##name (type _arg) \
    { this->m_##name = _arg; } \
  virtual type Get##name () const \
    { return this->m_##name; }

/**
 * Set macro for strings
 */
#define irisSetStringMacro(name) \
    virtual void Set##name (const std::string &_arg) \
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
 * Set macro for strings
 */
#define irisVirtualSetStringMacro(name) \
    virtual void Set##name (const std::string &_arg)  = 0;

/**
 * Get macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisVirtualGetStringMacro(name) \
    virtual const char *Get##name () const = 0;


/**
 * A get macro for boolean variables, IsXXX instead of GetXXX
 */
#define irisIsMacro(name) \
    virtual bool Is##name () const { \
    return this->m_##name; \
} 

/**
 * A get macro for boolean variables, IsXXX instead of GetXXX
 */
#define irisVirtualIsMacro(name) \
    virtual bool Is##name () const  = 0;

/**
  A macro combining the standard macros at the head of itk::Object classes
  */
#define irisITKObjectMacro(self,super) \
  typedef self Self; \
  typedef super Superclass; \
  typedef SmartPtr<Self> Pointer; \
  typedef SmartPtr<const Self> ConstPointer; \
  itkTypeMacro(self, super) \
  itkNewMacro(Self)

/**
  A macro combining the standard macros at the head of itk::Object classes,
  excluding the New construct
  */
#define irisITKAbstractObjectMacro(self,super) \
  typedef self Self; \
  typedef super Superclass; \
  typedef SmartPtr<Self> Pointer; \
  typedef SmartPtr<const Self> ConstPointer; \
  itkTypeMacro(self, super)

/** 
 * A rip off from the ITK not used macro to eliminate 'parameter not used'
 * warnings 
 */
#define irisNotUsed(x)

#endif // __SNAPCommonIO_h_
