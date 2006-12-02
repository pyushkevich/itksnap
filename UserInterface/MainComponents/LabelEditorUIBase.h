/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LabelEditorUIBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
  virtual void OnSetIdAction() = 0;
  virtual void OnMoveUpAction() = 0;
  virtual void OnMoveDownAction() = 0;
  virtual void OnCloseAction() = 0;
  virtual void OnLabelSelectAction() = 0;
  virtual void OnLabelPropertyChange() = 0;
};

#endif
