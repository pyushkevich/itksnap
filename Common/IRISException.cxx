/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISException.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 08:30:46 $
  Version:   $Revision: 1.4 $
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
#include "IRISException.h"
#include <cstdarg>
#include <stdio.h>

using namespace std;

IRISException::operator const char *() 
{
  return this->what();
}

IRISException::IRISException() 
  : exception()
{
  m_SimpleMessage = "Unspecified IRIS exception";
}

IRISException::IRISException(const char *message, ...)
  : exception()
{
  char buffer[1024];
  va_list args;
  va_start(args, message);
  vsprintf(buffer,message,args);
  va_end (args);
  m_SimpleMessage = buffer;
}


IRISWarning::IRISWarning()
  : IRISException()
{

}

IRISWarning::IRISWarning(const char *message, ...)
  : IRISException()
{
  char buffer[1024];
  va_list args;
  va_start(args, message);
  vsprintf(buffer,message,args);
  va_end (args);
  m_SimpleMessage = buffer;
}
