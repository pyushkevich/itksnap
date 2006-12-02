/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISException.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:09 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "IRISException.h"

IRISException
::operator const char *() 
{
  return m_SimpleMessage.c_str();
}

IRISException
::IRISException() 
{
  m_SimpleMessage = "Unspecified IRIS exception";
}

IRISException
::IRISException(const char *message) 
{
  m_SimpleMessage = message;
}

IRISException
::~IRISException() 
{
}

