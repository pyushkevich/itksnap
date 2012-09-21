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
      std::cout << "Considering " << wf << std::endl;
      if(wf && wf->IsInitialized())
        {
        std::cout << "  it is initialized! " << std::endl;

        iterator it = m_LayerMap.find(wf->GetUniqueId());
        if(it != m_LayerMap.end())
          {
          std::cout << "  it was found! " << std::endl;

          // Mark it as visited
          it->second.m_Visit = m_VisitCounter;
          }
        else
          {
          std::cout << "  it is new! " << std::endl;
          RHS rhs(m_Delegate.New(wf));
          rhs.m_Visit = m_VisitCounter;
          m_LayerMap.insert(std::make_pair(wf->GetUniqueId(), rhs));
          }
        }
      }
    }

  // Safely delete the objects that have not been visited
  iterator it = m_LayerMap.begin();
  while(it != m_LayerMap.end())
    {
    if(it->second.m_Visit != m_VisitCounter)
      {
      delete (TObject *) it->second;
      m_LayerMap.erase(it++);
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

template<class TObject, class TFilter, class TFactoryDelegate>
typename LayerAssociation<TObject, TFilter, TFactoryDelegate>::RHS &
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::operator [] (const TFilter *p)
{
  return m_LayerMap[p->GetUniqueId()];
}

template<class TObject, class TFilter, class TFactoryDelegate>
bool
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::HasLayer(const TFilter *p)
{
  std::cout << p << std::endl;
  if(p)
    std::cout << p->GetUniqueId() << std::endl;
  return p && m_LayerMap.find(p->GetUniqueId()) != m_LayerMap.end();
}


#endif // __Layer_Association_txx__
