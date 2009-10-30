/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ReorientImageUILogic.h,v $
  Language:  C++
  Date:      $Date: 2009/10/30 16:48:24 $
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

#ifndef __ReorientImageUILogic_h_
#define __ReorientImageUILogic_h_

#include "ReorientImageUI.h"
#include <vnl/vnl_matrix_fixed.h>

class UserInterfaceLogic;
class GenericImageData;

class ReorientImageUILogic : public ReorientImageUI
{
public:
  // Constructor
  ReorientImageUILogic();
  virtual ~ReorientImageUILogic() {}
  
  // Callbacks
  void OnDesiredRAIUpdate();
  void OnOkAction();
  void OnApplyAction();
  void OnCloseAction();

  void ShowDialog();
  void Register(UserInterfaceLogic *parent_ui);

  bool Shown();

private:

  void UpdateDesiredDerivedFields();

  // This code matches the orientation graphic to a RAI
  void UpdateOrientationGraphic(
    const char *rai, bool oblique, Fl_Wizard *grpDoll, Fl_Wizard *grpDollAxis[]);

  UserInterfaceLogic *m_ParentUI;
  GenericImageData *m_ImageData;

  // Direction matrix for the desired RAI code
  vnl_matrix_fixed<double, 3, 3> m_DesiredDirection;

  static const char m_RAICodes[3][2];
  static const char *m_AxisLabels[3][2];
};


#endif

