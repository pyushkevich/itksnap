/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef RLEImageRegionConstIterator_hxx
#define RLEImageRegionConstIterator_hxx

#include "RLEImageRegionConstIterator.h"

namespace itk
{

//----------------------------------------------------------------------------
// Increment when the fastest moving direction has reached its bound.
// This method should *ONLY* be invoked from the operator++() method.
template< typename TPixel, typename RunLengthCounterType>
void
ImageRegionConstIterator< RLEImage<TPixel, RunLengthCounterType> >
::Increment()
{
    // We have reached the end of the line (row), need to wrap around.
    m_Index[0] = m_BeginIndex[0];
    if (++m_Index[1] == m_BeginIndex[1] + m_Region.GetSize(1))
    {
        m_Index[1] = m_BeginIndex[1];
        m_Index[2]++;
    }
    SetIndex(m_Index);
}

//----------------------------------------------------------------------------
// Decrement when the fastest moving direction has reached its bound.
// This method should *ONLY* be invoked from the operator--() method.
template< typename TPixel, typename RunLengthCounterType>
void
ImageRegionConstIterator< RLEImage<TPixel, RunLengthCounterType> >
::Decrement()
{
    // We have reached the beginning of the line (row), need to wrap around.
    m_Index[0] = m_EndIndex[0];
    if (--m_Index[1] < m_BeginIndex[1])
    {
        m_Index[1] = m_EndIndex[1];
        m_Index[2]--;
    }
    SetIndex(m_Index);
}

} // end namespace itk

#endif //RLEImageRegionConstIterator_hxx
