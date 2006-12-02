/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LabelEditorUILogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
  void OnSetIdAction();
  void OnMoveUpAction();
  void OnMoveDownAction();
  void OnCloseAction();
  void OnLabelSelectAction();
  void OnLabelPropertyChange();

private:
  UserInterfaceBase *m_Parent;
  GlobalState *m_GlobalState;
  IRISApplication *m_Driver;

  // Internal functions
  int FindSpaceForNewLabel();
  size_t GetSelectedLabelId();

  // Quick access to color labels
  const ColorLabel &GetColorLabel(size_t iLabel);
  void SetColorLabel(size_t iLabel, const ColorLabel &xLabel);
};

#endif
