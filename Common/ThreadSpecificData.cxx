/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPLevelSetFunction.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "ThreadSpecificData.h"
#include "itkMultiThreader.h"
#ifndef DONT_USE_IRIS_EXCEPTION
  #include "IRISException.h"
#endif

#if defined(ITK_USE_PTHREADS)

void ThreadSpecificDataSupport::Deleter(void *p)
{
  free(p);
}

ThreadSpecificDataSupport::ThreadSpecificDataSupport()
{
  // Allocate storage for a key
  pthread_key_t *pkey = new pthread_key_t[1];

  // Create the key
  int rc = pthread_key_create(pkey, &ThreadSpecificDataSupport::Deleter);
  if(rc)
    {
    #ifndef DONT_USE_IRIS_EXCEPTION
      throw IRISException("pthread_key_create failed with rc = %d", rc);
    #else
      std::cerr << "pthread_key_create failed with rc =  " << rc;
    #endif  
    }

  // Store the key
  m_KeyPointer = pkey;
}

ThreadSpecificDataSupport::~ThreadSpecificDataSupport()
{
  // Delete the key
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  int rc = pthread_key_delete(pkey[0]);
  if(rc)
    {
    std::cerr << "pthread_key_delete failed with rc = " << rc;
    }
}

void *ThreadSpecificDataSupport::GetPtrCreateIfNeeded(size_t data_size)
{
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  void *pdata = pthread_getspecific(pkey[0]);
  if(!pdata)
    {
    pdata = malloc(data_size);
    int rc  = pthread_setspecific(pkey[0], pdata);
    if(rc)
      {
      #ifndef DONT_USE_IRIS_EXCEPTION
        throw IRISException("pthread_setspecific failed with rc = %d", rc);
      #else
        std::cerr << "pthread_setspecific failed with rc: " << rc;
      #endif  
      }
    }
  return pdata;
}

void *ThreadSpecificDataSupport::GetPtr() const
{
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  void *pdata = pthread_getspecific(pkey[0]);
  return pdata;
}

#elif defined(ITK_USE_WIN32_THREADS)

void ThreadSpecificDataSupport::Deleter(void *p)
{
}

ThreadSpecificDataSupport::ThreadSpecificDataSupport()
{
  DWORD *key = new DWORD[1];
  key[0] = TlsAlloc();
  if(key[0] == TLS_OUT_OF_INDEXES)
    {
    #ifndef DONT_USE_IRIS_EXCEPTION
      throw IRISException("TlsAlloc failed with error %d", GetLastError());
    #else
      std::cerr << "TlsAlloc failed with error: " << GetLastError();
    #endif
    }
  m_KeyPointer = key;
}

ThreadSpecificDataSupport::~ThreadSpecificDataSupport()
{
  DWORD *key = (DWORD *) m_KeyPointer;
  
  // Delete the stored stuff
  void *pdata = TlsGetValue(key[0]);
  if(pdata)
    free(pdata);

  TlsFree(key[0]);
  delete [] key;
}

void *ThreadSpecificDataSupport::GetPtrCreateIfNeeded(size_t data_size)
{
  DWORD *key = (DWORD *) m_KeyPointer;
  void *pdata = TlsGetValue(key[0]);

  if(!pdata)
    {
    if(GetLastError() != ERROR_SUCCESS)
      {
      #ifndef DONT_USE_IRIS_EXCEPTION
        throw IRISException("TlsGetValue failed with error %d", GetLastError());
      #else
        std::cerr << "TlsGetValue failed with error: " << GetLastError();
      #endif
      }
      
    pdata = malloc(data_size);
    if(!TlsSetValue(key[0], pdata))
      {
      #ifndef DONT_USE_IRIS_EXCEPTION
        throw IRISException("TlsSetValue failed with error %d", GetLastError());
      #else
        std::cerr << "TlsSetValue failed with error : " << GetLastError();
      #endif
      }  
    }

  return pdata;
}

void *ThreadSpecificDataSupport::GetPtr() const
{
  DWORD *key = (DWORD *) m_KeyPointer;
  void *pdata = TlsGetValue(key[0]);
  return pdata;
}


#else

#pragma warning("No support for non-pthread threads in ITK-SNAP yet!")

#endif

