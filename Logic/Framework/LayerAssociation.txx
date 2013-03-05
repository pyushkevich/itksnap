#ifndef __Layer_Association_txx__
#define __Layer_Association_txx__

#include "LayerAssociation.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "ImageWrapperBase.h"

template<class TObject, class TFilter, class TFactoryDelegate>
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::LayerAssociation()
{
  m_Source = NULL;
  m_VisitCounter = 0;
}

template<class TObject, class TFilter, class TFactoryDelegate>
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::~LayerAssociation()
{
  SetSource(NULL);
}


template<class TObject, class TFilter, class TFactoryDelegate>
void
LayerAssociation<TObject, TFilter, TFactoryDelegate>
::Update()
{
  m_VisitCounter++;

  // Iterate over all the objects in the image data
  if(m_Source)
    {
    // Iterate over all of the image data objects in IRISApplication
    GenericImageData *id[] = {m_Source->GetIRISImageData(),
                              m_Source->GetSNAPImageData()};

    for(int k = 0; k < 2; k++)
      {
      if(id[k])
        {
        for(LayerIterator lit = id[k]->GetLayers(); !lit.IsAtEnd(); ++lit)
          {
          ImageWrapperBase *wb = lit.GetLayer();
          TFilter *wf = dynamic_cast<TFilter *>(wb);
          if(wf && wf->IsInitialized())
            {
            iterator it = m_LayerMap.find(wf->GetUniqueId());
            if(it != m_LayerMap.end())
              {
              // Mark it as visited
              it->second.m_Visit = m_VisitCounter;
              }
            else
              {
              RHS rhs(m_Delegate.New(wf));
              rhs.m_Visit = m_VisitCounter;
              m_LayerMap.insert(std::make_pair(wf->GetUniqueId(), rhs));
              }
            }
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
::SetSource(IRISApplication *source)
{
  if(source != m_Source)
    {
    m_Source = source;
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
  return p && m_LayerMap.find(p->GetUniqueId()) != m_LayerMap.end();
}


#endif // __Layer_Association_txx__
