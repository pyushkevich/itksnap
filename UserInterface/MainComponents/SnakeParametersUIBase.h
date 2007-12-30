/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParametersUIBase.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:18 $
  Version:   $Revision: 1.2 $
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
#ifndef __SnakeParametersUIBase_h_
#define __SnakeParametersUIBase_h_

class Fl_Valuator;
class Fl_Check_Button;

/**
 * \class SnakeParametersUIBase
 * \brief Base class for the parameter setting user interface.
 */
class SnakeParametersUIBase 
{
public:
  virtual ~SnakeParametersUIBase() {}
  virtual void OnAdvectionExponentChange(Fl_Valuator *input) = 0;
  virtual void OnAdvectionWeightChange(Fl_Valuator *input) = 0;
  virtual void OnCurvatureExponentChange(Fl_Valuator *input) = 0;
  virtual void OnCurvatureWeightChange(Fl_Valuator *input) = 0;
  virtual void OnPropagationExponentChange(Fl_Valuator *input) = 0;
  virtual void OnPropagationWeightChange(Fl_Valuator *input) = 0;
  virtual void OnHelpAction() = 0 ;
  virtual void OnOkAction() = 0;
  virtual void OnCloseAction() = 0;
  virtual void OnSaveParametersAction() = 0;
  virtual void OnLoadParametersAction() = 0;
  virtual void OnAdvancedEquationAction() = 0;  

  // Advanced page
  virtual void OnTimeStepAutoAction() = 0;
  virtual void OnTimeStepChange(Fl_Valuator *input) = 0;
  virtual void OnSmoothingWeightChange(Fl_Valuator *input) = 0;
  virtual void OnLegacyClampChange(Fl_Check_Button *input) = 0;
  virtual void OnLegacyGroundChange(Fl_Valuator *input) = 0;
  virtual void OnSolverChange() = 0;

  // Force example
  virtual void OnAnimateAction() = 0;

  // Help System
  virtual void ShowHelp(const char *link) = 0;
};

#endif // __SnakeParametersUIBase_h_
