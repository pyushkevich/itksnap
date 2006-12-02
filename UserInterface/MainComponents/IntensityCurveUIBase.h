/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IntensityCurveUIBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IntensityCurveUIBase_h_
#define __IntensityCurveUIBase_h_

/**
 * \class IntensityCurveUIBase
 * \brief Base class for intensity curve FLTK interface.
 */
class IntensityCurveUIBase {
public:
    virtual ~IntensityCurveUIBase() {}
  // Callbacks made from the user interface
  virtual void OnClose() = 0;
  virtual void OnReset() = 0;
  virtual void OnWindowLevelChange() = 0;
  virtual void OnControlPointNumberChange() = 0;
  virtual void OnUpdateHistogram() = 0;
};

#endif // __IntensityCurveUIBase_h_
