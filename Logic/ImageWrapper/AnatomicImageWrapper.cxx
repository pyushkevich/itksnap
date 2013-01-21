#include "AnatomicImageWrapper.h"

AnatomicImageWrapper::AnatomicImageWrapper()
{
}

ImageWrapperBase::DisplaySlicePointer
AnatomicImageWrapper::GetDisplaySlice(unsigned int dim)
{
  if(m_DisplayMode.UsePalette)
    {
    return this->GetComponentWrapper(m_DisplayMode.ActiveChannel)->GetDisplaySlice[dim];
    }
  else
    {

    }
}
