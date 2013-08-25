/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LabelEditorUILogic.h,v $
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
#ifndef __LabelEditorUILogic
#define __LabelEditorUILogic

#include "LabelEditorUI.h"
#include "ColorLabel.h"
#include <map>
#include <vector>

// Forward references to some classes
class IRISApplication;
class GlobalState;
class UserInterfaceBase;

/**
 * \class LabelEditorUILogic
 * \brief Logic class for Label editor UI logic
 */
class LabelEditorUILogic : public LabelEditorUI {
public:
  LabelEditorUILogic();
  virtual ~LabelEditorUILogic();

  /** Register with the parent user interface */
  void Register(UserInterfaceBase *parent);

  /** Set the active label */
  void SetEditorLabel(unsigned int iLabel);

  /** Respond to an update in the list of labels, setting the current 
   * label to iLabel */
  void OnLabelListUpdate(unsigned int iCurrentLabel);

  /** Display the editor window */
  void DisplayWindow();
  
  // Callbacks from the user interface
  void OnNewAction();
  void OnDuplicateAction();
  void OnDeleteAction();
  void OnDeleteAllAction();
  void OnSetIdAction();
  void OnMoveUpAction();
  void OnMoveDownAction();
  void OnCloseAction();
  void OnLabelSelectAction();
  void OnLabelPropertyChange();
  void OnToolsDialogAction();
  void OnToolsApplyAction();
  void OnToolsCloseAction();
  void OnToolsOperationChange();

private:
  UserInterfaceBase *m_Parent;
  GlobalState *m_GlobalState;
  IRISApplication *m_Driver;

  // Internal menus
  static const size_t TOOL_PAGE_LABEL_MENU_COUNT;
  Fl_Menu_Item **m_ToolPageLabelMenu;

  // Internal functions
  int FindSpaceForNewLabel();
  size_t GetSelectedLabelId();

  // Quick access to color labels
  const ColorLabel &GetColorLabel(size_t iLabel);
  void SetColorLabel(size_t iLabel, const ColorLabel &xLabel);
};

#endif
