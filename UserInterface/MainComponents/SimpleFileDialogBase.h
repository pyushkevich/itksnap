/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SimpleFileDialogBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:23 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SimpleFileDialogBase_h_
#define __SimpleFileDialogBase_h_

class SimpleFileDialogBase {
public:
    virtual ~SimpleFileDialogBase() {}
  virtual void OnFileChange() = 0;
  virtual void OnHistoryChange() = 0;
  virtual void OnBrowseAction() = 0;
  virtual void OnOkAction() = 0;
  virtual void OnCancelAction() = 0;
};

#endif // __SimpleFileDialogBase_h_
