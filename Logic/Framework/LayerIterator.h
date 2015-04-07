#ifndef LAYERITERATOR_H
#define LAYERITERATOR_H

#include "SNAPCommon.h"
#include <map>
#include <vector>

class GenericImageData;
class ImageWrapperBase;
class ScalarImageWrapperBase;
class VectorImageWrapperBase;

/**
 * An iterator for moving through layers in the GenericImageData class
 */
class LayerIterator
{
public:

  LayerIterator(GenericImageData *data, int role_filter = ALL_ROLES);

  bool IsAtEnd() const;

  // Move to the end
  LayerIterator &MoveToBegin();

  // Move to the end
  LayerIterator &MoveToEnd();

  // Move to a specific layer, or end if the layer is not found
  LayerIterator &Find(ImageWrapperBase *value);

  LayerIterator &Find(unsigned long layer_id);

  LayerIterator & operator++();

  LayerIterator & operator+=(int k);

  /** Get the layer being pointed to */
  ImageWrapperBase *GetLayer() const;

  /** Get the layer being pointed to, cast as Scalar (or NULL) */
  ScalarImageWrapperBase *GetLayerAsScalar() const;

  /** Get the layer being pointed to, cast as Gray (or NULL) */
  VectorImageWrapperBase *GetLayerAsVector() const;

  /** Get the role of the current layer */
  LayerRole GetRole() const;

  /** Get the position of the current layer within its role */
  int GetPositionInRole() const;

  /** Get the number of layers in the role of current layer */
  int GetNumberOfLayersInRole();

  /** Check if this is the first/last layer in its role */
  bool IsFirstInRole() const;
  bool IsLastInRole() const;

  void Print(const char *) const;

  /** Compare two iterators */
  bool operator == (const LayerIterator &it);
  bool operator != (const LayerIterator &it);

private:

  typedef std::vector<SmartPtr<ImageWrapperBase> > WrapperList;
  typedef WrapperList::const_iterator WrapperListIterator;

  typedef std::map<LayerRole, WrapperList> WrapperRoleMap;
  typedef WrapperRoleMap::iterator WrapperRoleIterator;

  // Pointer to the parent data
  GenericImageData *m_ImageData;

  // The filter defining which roles to iterate
  int m_RoleFilter;

  // A pair of iterators that define the state of this iterator
  WrapperRoleIterator m_RoleIter;
  WrapperListIterator m_WrapperInRoleIter;

  // Internal method that advances the internal iterators by one step,
  // regardless of whether that makes the iterator point to a valid layer
  // or not
  void MoveToNextTrialPosition();

  // Check if the iterator is pointing to a valid layer
  bool IsPointingToListableLayer() const;

  // Default names for wrappers
  static std::map<LayerRole, std::string> m_RoleDefaultNames;
};



#endif // LAYERITERATOR_H
