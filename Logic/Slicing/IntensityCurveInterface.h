/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveInterface.h,v $
  Language:  C++
  Date:      $Date: 2009/09/10 19:44:50 $
  Version:   $Revision: 1.3 $
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
#ifndef __IntensityCurveInterface_h_
#define __IntensityCurveInterface_h_

#include <itkDataObject.h>
#include <itkObjectFactory.h>
#include <Registry.h>
#include <SNAPEvents.h>

/**
 * \class IntensityCurveInterface
 * \brief The base class for intensity mapping splines
 */
class ITK_EXPORT IntensityCurveInterface : public itk::DataObject
{
public:

  /** Standard class typedefs. */
  typedef IntensityCurveInterface Self;
  typedef itk::DataObject Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(IntensityCurveInterface,itk::DataObject)

  /**
   * Initialize the spline with initial number of control points.
   * The spline will be a linear mapping from (0,0) to (1,1)
   */
  virtual void Initialize(unsigned int nControlPoints = 3) = 0;

  /**
   * Reset to linear mapping, keeping number of points intact
   */
  virtual void Reset() { this->Initialize(this->GetControlPointCount()); }

  // Check if the curve is in default state (linear from 0 to 1)
  virtual bool IsInDefaultState() = 0;

  /**
   * Get the value of a control point
   */
  virtual void GetControlPoint(unsigned int iControlPoint, 
    float &t, float &x) const = 0;

  /**
   * Get the value of a control point
   */
  virtual unsigned int GetControlPointCount() const = 0;

  /**
   * Update the value of a control point
   */
  virtual void UpdateControlPoint(unsigned int iControlPoint, 
    float t, float x) = 0;

  /**
   * This method linearly maps the t-values of all the control points to 
   * the range between tMin and tMax.  It's used for intensity windowing 
   * where we want to adjust the domain of the curve without changing its 
   * shape
   */
  virtual void ScaleControlPointsToWindow(float tMin, float tMax) = 0;

  /**
   * Check the monotonicity of the spline curve
   */
  virtual bool IsMonotonic() const = 0;

  /** Load the curve from a registry object */
  virtual void LoadFromRegistry(Registry &registry) = 0;

  /** Save the curve to a registry object */
  virtual void SaveToRegistry(Registry &registry) const = 0;

  /** Evaluate the curve */
  virtual float Evaluate(const float &t) const = 0;

protected:
  IntensityCurveInterface(){};
  virtual ~IntensityCurveInterface(){};

private:
  IntensityCurveInterface(const Self& ); //purposely not implemented
  void operator=(const Self& ); //purposely not implemented
};

#endif // __IntensityCurveInterface_h_
