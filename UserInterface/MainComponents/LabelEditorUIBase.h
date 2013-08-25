/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LabelEditorUIBase.h,v $
  Language:  C++
  Date:      $Date: 2008/11/17 19:38:23 $
  Version:   $Revision: 1.4 $
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
#ifndef __LabelEditorUIBase_h_
#define __LabelEditorUIBase_h_

/**
 * \class LabelEditorUIBase
 * \brief Base class for Label editor UI logic
 */
class LabelEditorUIBase {
public:
    virtual ~LabelEditorUIBase() {}
  // Callbacks
  virtual void OnNewAction() = 0;
  virtual void OnDuplicateAction() = 0;
  virtual void OnDeleteAction() = 0;
  virtual void OnDeleteAllAction() = 0;
  virtual void OnSetIdAction() = 0;
  virtual void OnMoveUpAction() = 0;
  virtual void OnMoveDownAction() = 0;
  virtual void OnCloseAction() = 0;
  virtual void OnLabelSelectAction() = 0;
  virtual void OnLabelPropertyChange() = 0;
  virtual void OnToolsDialogAction() = 0;
  virtual void OnToolsApplyAction() = 0;
  virtual void OnToolsCloseAction() = 0;
  virtual void OnToolsOperationChange() = 0;
};

#endif
