/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveUIBase.h,v $
  Language:  C++
  Date:      $Date: 2008/02/10 23:55:22 $
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
  virtual void OnAutoFitWindow() = 0;
  virtual void OnWindowLevelChange() = 0;
  virtual void OnControlPointNumberChange() = 0;
  virtual void OnUpdateHistogram() = 0;
};

#endif // __IntensityCurveUIBase_h_
