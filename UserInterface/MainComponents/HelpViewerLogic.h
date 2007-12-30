/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: HelpViewerLogic.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
  Version:   $Revision: 1.2 $
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
#ifndef __HelpViewerLogic_h_
#define __HelpViewerLogic_h_

#include "HelpViewer.h"
#include "SNAPCommonUI.h"

#include <list>


/**
 * \class HelpViewerLogic
 * \brief UI logic for a help viewer window 
 */ 
class HelpViewerLogic : public HelpViewer {
public:
  
  HelpViewerLogic();
  virtual ~HelpViewerLogic();

  /** Set the 'Contents' link */
  irisSetStringMacro(ContentsLink);

  /** Display the help window on a particular page */
  void ShowHelp(const char *url);
  
  void OnLinkAction();
  void OnBackAction();
  void OnForwardAction();
  void OnCloseAction();
  void OnContentsAction();

  // Callback for when a link is followed
  const char *LinkCallback(const char *uri);

private:
  // The list of visited links (back and forth)
  typedef std::list<std::string> LinkListType;
  LinkListType m_LinkList;

  // The current place on the list
  typedef LinkListType::iterator LinkIterator;
  LinkIterator m_Iterator;
  
  // Contents page
  std::string m_ContentsLink;
  
  // A method to push a URL onto the history stack
  void PushURL(const char *url);
};

#endif // __HelpViewerLogic_h_

