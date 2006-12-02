/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: HelpViewerBase.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __HelpViewerBase_h_
#define __HelpViewerBase_h_

class HelpViewerBase {
public:
    virtual ~HelpViewerBase () {}
  virtual void OnLinkAction() = 0;
  virtual void OnBackAction() = 0;
  virtual void OnForwardAction() = 0;
  virtual void OnCloseAction() = 0;
  virtual void OnContentsAction() = 0;
};

#endif // __HelpViewerBase_h_
