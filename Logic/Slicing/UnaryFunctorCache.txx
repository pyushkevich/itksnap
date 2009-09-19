/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UnaryFunctorCache.txx,v $
  Language:  C++
  Date:      $Date: 2009/09/19 07:47:03 $
  Version:   $Revision: 1.5 $
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
#include "UnaryFunctorCache.h"
#include "itkSimpleFastMutexLock.h"

template <class TInput, class TOutput, class TFunctor>
UnaryFunctorCache<TInput,TOutput,TFunctor>
::UnaryFunctorCache() 
: m_CachingFunctor(this)
{
  m_InputFunctor = NULL;
  m_Cache = NULL;
  m_CacheBegin = 0;
  m_CacheLength = 0;
  m_CacheAllocatedLength = 0;
  m_Modified = false;
}

template <class TInput, class TOutput, class TFunctor>
UnaryFunctorCache<TInput,TOutput,TFunctor>
::~UnaryFunctorCache() 
{
  if(m_Cache)
    delete[] m_Cache;
}

template <class TInput, class TOutput, class TFunctor>
void 
UnaryFunctorCache<TInput,TOutput,TFunctor>
::ComputeCache() 
{
  // Can't have this running in a thread
  static itk::SimpleFastMutexLock mutex;
  mutex.Lock();

  // Functor must be declared and cache length must be non-zero
  assert(m_InputFunctor && m_CacheLength > 0);

  // Allocate the cache, but only if the length changed
  if(m_CacheAllocatedLength != m_CacheLength)
    {
    if(m_Cache) 
      delete m_Cache;
    m_Cache = new TOutput[m_CacheLength];
    m_CacheAllocatedLength = m_CacheLength;
    }
    
  // Compute the cache elements
  int iFunc = m_CacheBegin;
  for(unsigned int iCache = 0; iCache < m_CacheLength; iCache++)
    {
    m_Cache[iCache] = (*m_InputFunctor)(iFunc++);
    }

  m_Modified = false;

  // Back to thread safety
  mutex.Unlock();
}

template <class TInput, class TOutput, class TFunctor>
void 
UnaryFunctorCache<TInput,TOutput,TFunctor>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Cache Start: " << m_CacheBegin << std::endl;
  os << indent << "Cache Length: " << m_CacheLength << std::endl;
}

