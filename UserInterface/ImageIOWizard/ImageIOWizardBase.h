/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageIOWizardBase.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
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
