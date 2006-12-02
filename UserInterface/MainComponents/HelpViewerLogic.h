/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: HelpViewerLogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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

