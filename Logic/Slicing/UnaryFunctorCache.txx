/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UnaryFunctorCache.txx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#include "UnaryFunctorCache.h"

template <class TInput, class TOutput, class TFunctor>
UnaryFunctorCache<TInput,TOutput,TFunctor>
::UnaryFunctorCache() 
: m_CachingFunctor(this)
{
  m_InputFunctor = NULL;
  m_Cache = NULL;
  m_CacheBegin = 0;
  m_CacheLength = 0;
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
  // Functor must be declared and cache length must be non-zero
  assert(m_InputFunctor && m_CacheLength > 0);

  // Allocate the cache
  if(m_Cache)
    delete[] m_Cache;
  m_Cache = new TOutput[m_CacheLength];

  // Compute the cache elements
  for(unsigned int iCache = 0, iFunc = m_CacheBegin;
    iCache < m_CacheLength;
    iCache++,iFunc++)
    {
    m_Cache[iCache] = (*m_InputFunctor)(iFunc);
    }
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

