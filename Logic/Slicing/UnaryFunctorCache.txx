/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: UnaryFunctorCache.txx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:15 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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

