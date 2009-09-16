/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISException.h,v $
  Language:  C++
  Date:      $Date: 2009/09/16 20:03:13 $
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
#ifndef __IRIS_Exceptions_h_
#define __IRIS_Exceptions_h_

#include "SNAPCommon.h"

using std::string;

/** 
 * \class IRISException
 * \brief Sets up a family of SNAP/IRIS exceptions
 */
class IRISException {
protected:
  string m_SimpleMessage;

public:
  IRISException();
  IRISException(const char *message, ...);

  virtual ~IRISException();

  const char * what() { return m_SimpleMessage.c_str(); }

  operator const char *();
};

/**
 * Set macro borrowed from VTK and modified.  Assumes m_ for private vars
 */
#define irisExceptionMacro(name,parent) \
class name : public parent { \
public: \
        name() : parent() {} \
          name(const char *message) : parent(message) {} \
          virtual ~name() {} \
};

irisExceptionMacro(IRISExceptionIO,IRISException)

#endif
