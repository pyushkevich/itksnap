/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: HelpViewerLogic.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:17 $
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
#include "HelpViewerLogic.h"
#include <FL/Fl_Shared_Image.H>
#include <FL/filename.H>

using namespace std;

// Callback for when links are clicked
const char *HelpViewerLogicLinkCallback(Fl_Widget *w, const char *uri)
{
  // Only deal with visited html
  const char *ext = fl_filename_ext(uri);
  if(strcmp(ext,".html") != 0 && strcmp(ext,".HTML") != 0)
    return uri;

  // Pass on to the class
  HelpViewerLogic *hvl = static_cast<HelpViewerLogic *>(w->user_data());
  return hvl->LinkCallback(uri);  
}

HelpViewerLogic
::HelpViewerLogic()
{
  // Make sure that Fl is set up for GIF/PNG images
  fl_register_images();

  // Clear the linked list
  m_Iterator = m_LinkList.begin();  
}
  
HelpViewerLogic
::~HelpViewerLogic()
{
}

void 
HelpViewerLogic
::ShowHelp(const char *url)
{
  // Put the url into the history list
  PushURL(url);
  
  // Configure the browser
  m_BrsHelp->link(HelpViewerLogicLinkCallback);
  m_BrsHelp->user_data(this);
  m_BrsHelp->load(url); 

  // Show the window
  if(!m_WinHelp->shown())
    m_WinHelp->show();
}

const char *
HelpViewerLogic
::LinkCallback(const char *url)
{
  // Put the url into the history list
  PushURL(url);

  // Return the URL unchanged
  return url;
}

void 
HelpViewerLogic
::PushURL(const char *url)
{
  // If the link is the same as our current location, don't do anything
  if(m_Iterator != m_LinkList.end() && 
     0 == strcmp(m_Iterator->c_str(),url))
    return;

  // Clear the forward stack
  if(m_Iterator != m_LinkList.end())
    m_LinkList.erase(++m_Iterator,m_LinkList.end());

  // Add the new link to the list
  m_LinkList.push_back(string(url));

  // Point to the end of the list
  m_Iterator = m_LinkList.end();
  m_Iterator--;

  // Disable the forward button
  m_BtnForward->deactivate();

  // Activate the back button if there is a back link
  if(m_Iterator != m_LinkList.begin())
    m_BtnBack->activate();
  else
    m_BtnBack->deactivate();    
}

void 
HelpViewerLogic
::OnLinkAction()
{
}

void 
HelpViewerLogic
::OnBackAction()
{
  // Can't be at the head of the list
  assert(m_Iterator != m_LinkList.begin());
  
  // Go back with the iterator
  --m_Iterator;

  // Enable the forward button
  m_BtnForward->activate();

  // Perhaps disable the back button
  if(m_Iterator == m_LinkList.begin())
    m_BtnBack->deactivate();

  // Show the current link
  m_BrsHelp->load(m_Iterator->c_str());
}

void 
HelpViewerLogic
::OnForwardAction()
{
  // Can't be at the end of the list
  assert(++m_Iterator != m_LinkList.end());

  // Enable the back button
  m_BtnBack->activate();

  // Perhaps disable the forward button
  LinkIterator itTest = m_Iterator;
  if(++itTest == m_LinkList.end())
    m_BtnForward->deactivate();

  // Show the current link
  m_BrsHelp->load(m_Iterator->c_str());
}

void 
HelpViewerLogic
::OnCloseAction()
{
  m_WinHelp->hide();
}

void 
HelpViewerLogic
::OnContentsAction()
{
  // Go to the contents page
  ShowHelp(m_ContentsLink.c_str());  
}


