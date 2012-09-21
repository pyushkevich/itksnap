#ifndef LAYERASSOCIATION_H
#define LAYERASSOCIATION_H

#include <SNAPCommon.h>

class ImageWrapperBase;
class GenericImageData;

template<class TObject, class TLayer>
struct DefaultLayerAssociationFactoryDelegate
{
  TObject * New(TLayer *layer) { return new TObject(layer); }
};

template <class TObject>
struct LayerAssociationObjectWrapper
{
  operator TObject * () { return m_Object; }

  LayerAssociationObjectWrapper(TObject *p = NULL)
  {
    m_Object = p;
    m_Visit = 0;
  }

  TObject *m_Object;
  unsigned long m_Visit;
};

/**
  YAY! My code is staring to look like Boost! The pro: I have finally figured
  out the generic programming paradigm. The con: I will be coding ITK-SNAP
  forever...

  Seriously, this is an object than maintains a one-to-one association between
  layers in a GenericImageData object, and objects of some user-defined type.
  Calling Update() will create new objects for new layers that have appeared
  and delete objects associated with layers that have disappeared.

  The user can pass in a delegate factory object, which can be used to create
  new instances of the Object.
  */
template<class TObject,
         class TFilter = ImageWrapperBase,
         class TFactoryDelegate =
           DefaultLayerAssociationFactoryDelegate<TObject, TFilter> >
class LayerAssociation
{
public:
  typedef LayerAssociation<TObject, TFilter> Self;
  typedef LayerAssociationObjectWrapper<TObject> RHS;

  LayerAssociation();
  virtual ~LayerAssociation();

  void SetImageData(GenericImageData *data);

  // Set the delegate, provided in case you need a delegate with some
  // special functionality.
  irisSetMacro(Delegate, const TFactoryDelegate &)
  irisGetMacro(Delegate, const TFactoryDelegate &)

  // Allow lookup by layer directly
  RHS & operator [] (const TFilter *p);

  bool HasLayer(const TFilter *p);

  void Update();

protected:
  typedef std::map<unsigned long, RHS> LayerMap;
  typedef typename LayerMap::iterator iterator;
  typedef typename LayerMap::const_iterator const_iterator;

  LayerMap m_LayerMap;
  GenericImageData *m_ImageData;
  TFactoryDelegate m_Delegate;
  unsigned long m_VisitCounter;

};


#endif // LAYERASSOCIATION_H
