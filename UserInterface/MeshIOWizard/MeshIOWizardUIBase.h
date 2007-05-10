/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: MeshIOWizardUIBase.h,v $
  Language:  C++
  Date:      $Date: 2007/05/10 20:19:51 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __MeshIOWizardUIBase_h_
#define __MeshIOWizardUIBase_h_

/**
 * \class MeshIOWizardUIBase
 * The base class for the Mesh Export Wizard UI.
 */
class MeshIOWizardUIBase {
public:
  // Dummy constructor and destructor
  MeshIOWizardUIBase() {};
  virtual ~MeshIOWizardUIBase() {};

  virtual void OnCancel() = 0;
  virtual void OnFilePageBrowse() = 0;
  virtual void OnFilePageNext() = 0;
  virtual void OnFilePageFileInputChange() = 0;
  virtual void OnFilePageFileHistoryChange() = 0;
  virtual void OnFilePageFileFormatChange() = 0;
  virtual void OnFilePageBack() = 0;

  virtual void OnMeshPageNext() = 0;
  virtual void OnMeshPageRadioChange() = 0;
};

#endif // __MeshIOWizardUIBase_h_

