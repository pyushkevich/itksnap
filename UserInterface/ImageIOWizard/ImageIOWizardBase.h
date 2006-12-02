/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageIOWizardBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageIOWizardBase_h_
#define __ImageIOWizardBase_h_

/**
 * \class ImageIOWizardBase
 * The base class for the Image IO Wizard window.  This class declares 
 * callback functions and the child of ImageIOWizard implements them
 */
class ImageIOWizardBase {
public:
    virtual ~ImageIOWizardBase() {}
  virtual void OnCancel() = 0;
  virtual void OnFilePageBrowse() = 0;
  virtual void OnFilePageNext() = 0;
  virtual void OnFilePageFileInputChange() = 0;
  virtual void OnFilePageFileHistoryChange() = 0;
  virtual void OnFilePageFileFormatChange() = 0;
  virtual void OnHeaderPageNext() = 0;
  virtual void OnHeaderPageBack() = 0;
  virtual void OnHeaderPageInputChange() = 0;
  virtual void OnDICOMPageNext() = 0;
  virtual void OnDICOMPageBack() = 0;
  virtual void OnOrientationPageNext() = 0;
  virtual void OnOrientationPageBack() = 0;
  virtual void OnOrientationPageSelectPreset() = 0;
  virtual void OnOrientationPageSelect() = 0;
  virtual void OnOrientationPageRAIChange() = 0;
  virtual void OnSummaryPageFinish() = 0;
  virtual void OnSummaryPageBack() = 0;

  // Save related functions
  virtual void OnSaveFilePageFileInputChange() = 0;
  virtual void OnSaveFilePageFileFormatChange() = 0;
  virtual void OnSaveFilePageFileHistoryChange() = 0;
  virtual void OnSaveFilePageBrowse() = 0;
  virtual void OnSaveFilePageSave() = 0;
  virtual void OnSaveCancel() = 0;
};

#endif
