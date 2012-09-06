#ifndef __Layer_Association_txx__
#define __Layer_Association_txx__

#include "LayerAssociation.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"

template<class TObject, class TFilter, class TFactoryDelegate>
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::LayerAssociation()
{
  m_ImageData = NULL;
  m_VisitCounter = 0;
}

template<class TObject, class TFilter, class TFactoryDelegate>
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::~LayerAssociation()
{
  SetImageData(NULL);
}

template<class TObject, class TFilter, class TFactoryDelegate>
void
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::Update()
{
  m_VisitCounter++;

  // Iterate over all the objects in the image data
  if(m_ImageData)
    {
    for(LayerIterator lit = m_ImageData->GetLayers(); !lit.IsAtEnd(); ++lit)
      {
      ImageWrapperBase *wb = lit.GetLayer();
      TFilter *wf = dynamic_cast<TFilter *>(wb);
      if(wf)
        {
        iterator it = this->find(wf);
        if(it != this->end())
          {
          // Mark it as visited
          it->second.m_Visit = m_VisitCounter;
          }
        else
          {
          RHS rhs(m_Delegate.New(wf));
          rhs.m_Visit = m_VisitCounter;
          this->insert(std::make_pair(wf, rhs));
          }
        }
      }
    }

  // Safely delete the objects that have not been visited
  iterator it = this->begin();
  while(it != this->end())
    {
    if(it->second.m_Visit != m_VisitCounter)
      {
      delete (TObject *) it->second;
      this->erase(it++);
      }
    else
      it++;
    }
}

template<class TObject, class TFilter, class TFactoryDelegate>
void
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::SetImageData(GenericImageData *data)
{
  if(data != m_ImageData)
    {
    m_ImageData = data;
    this->Update();
    }
}

#endif // __Layer_Association_txx__
