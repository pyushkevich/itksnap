#ifndef IMAGEANNOTATIONDATA_H
#define IMAGEANNOTATIONDATA_H

#include "SNAPCommon.h"
#include <utility>
#include <string>
#include <list>
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "TagList.h"

class Registry;

namespace annot
{

typedef Vector3d Point;

/**
 * @brief Slices where the annotation is displayed
 */
enum VisSlice {
  SINGLE_SLICE = 0, ALL_SLICES
};

/**
  @brief Types of annotations
 */
enum AnnotationType {
  LINE_SEGMENT=0, LANDMARK, UNKNOWN_ANNOTATION
};

/**
 * @brief Parent class for annotations
 */
class AbstractAnnotation : public itk::DataObject
{
public:
  irisITKAbstractObjectMacro(AbstractAnnotation, itk::DataObject)

  /** Whether this annotation is currently selected */
  irisGetSetMacro(Selected, bool)

  /** Whether this annotation is visible in all slices or just its own slice */
  irisGetSetMacro(VisibleInAllSlices, bool)

  /** Whether this annotation is visible in all ortho planes or just its own plane */
  irisGetSetMacro(VisibleInAllPlanes, bool)

  /** The image dimension to which this annotation belongs, or -1 if it's non-planar */
  irisGetSetMacro(Plane, int)

  /** Get the color of the annotation */
  irisGetSetMacro(Color, const Vector3d &)

  /** Get the tags in the annotation */
  irisGetSetMacro(Tags, const TagList &)

  /** Get color as a u-int vector */
  Vector3ui GetColor3ui() const;
  void SetColor3ui(const Vector3ui &color);

  /** Test whether the annotation is visible in the current plane and given slice */
  bool IsVisible(int plane, int slice) const;

  /** Test whether the annotation is visible in the current plane in some slice */
  bool IsVisible(int plane) const;

  /** Get the slice this annotation belongs to for a particular plane */
  virtual int GetSliceIndex(int plane) const = 0;

  /** Get the anchor point for the annotation in a given plane - used for sorting */
  virtual Vector3d GetAnchorPoint(int plane) const = 0;

  /** Get the 3D center of the annotation */
  virtual Vector3d GetCenter() const = 0;

  /** Get the annotation type */
  virtual AnnotationType GetType() const = 0;

  /** Move the annotation by given amount in physical space */
  virtual void MoveBy(const Vector3d &offset) = 0;

  /** Save the annotation data to registry */
  virtual void Save(Registry &folder);

  /** Load from the registry */
  virtual void Load(Registry &folder);

  /** Get the unique id of this tag */
  virtual unsigned long GetUniqueId() const;

protected:

  AbstractAnnotation();
  ~AbstractAnnotation() {}

  // Unique Id of this annotation, may not be zero
  unsigned long m_UniqueId;

  bool m_Selected;
  bool m_VisibleInAllSlices;
  bool m_VisibleInAllPlanes;

  Vector3d m_Color;

  // Each annotation can have tags
  TagList m_Tags;

  int m_Plane;
};

/** A simple line segment */
typedef std::pair<Vector3d,Vector3d>           LineSegment;

class LineSegmentAnnotation : public AbstractAnnotation
{
public:
  irisITKObjectMacro(LineSegmentAnnotation, AbstractAnnotation)

  typedef LineSegment                   ObjectType;

  irisGetSetMacro(Segment, const LineSegment &)

  virtual void Save(Registry &folder) ITK_OVERRIDE;
  virtual void Load(Registry &folder) ITK_OVERRIDE;

  virtual void MoveBy(const Vector3d &offset) ITK_OVERRIDE;
  virtual Vector3d GetCenter() const ITK_OVERRIDE;
  virtual AnnotationType GetType() const ITK_OVERRIDE { return LINE_SEGMENT; }


protected:

  virtual int GetSliceIndex(int plane) const ITK_OVERRIDE;

  virtual Vector3d GetAnchorPoint(int plane) const ITK_OVERRIDE;

  LineSegment m_Segment;
};

/**
 * @brief Text with location
 */
struct Landmark
{
  std::string Text;
  Vector3d Pos;
  Vector2d Offset;
};

class LandmarkAnnotation : public AbstractAnnotation
{
public:
  irisITKObjectMacro(LandmarkAnnotation, AbstractAnnotation)

  typedef Landmark                   ObjectType;

  irisGetSetMacro(Landmark, const Landmark &)

  virtual void MoveBy(const Vector3d &offset) ITK_OVERRIDE;
  virtual Vector3d GetCenter() const ITK_OVERRIDE;
  virtual AnnotationType GetType() const ITK_OVERRIDE { return LANDMARK; }

protected:

  virtual int GetSliceIndex(int plane) const ITK_OVERRIDE;
  virtual Vector3d GetAnchorPoint(int plane) const ITK_OVERRIDE;

  virtual void Save(Registry &folder) ITK_OVERRIDE;
  virtual void Load(Registry &folder) ITK_OVERRIDE;



  Landmark m_Landmark;
};

}


/**
 * This class describes a collection of image annotations
 *
 * Image annotations are defined in voxel coordinate space. This helps keep the
 * annotations in place when header information changes. It also makes the internal
 * logic simpler.
 */
class ImageAnnotationData : public itk::DataObject
{
public:
  typedef annot::AbstractAnnotation AbstractAnnotation;
  typedef SmartPtr<AbstractAnnotation> AnnotationPtr;
  typedef std::list<AnnotationPtr> AnnotationList;
  typedef AnnotationList::iterator AnnotationIterator;
  typedef AnnotationList::const_iterator AnnotationConstIterator;

  irisITKObjectMacro(ImageAnnotationData, itk::DataObject)

  irisGetMacro(Annotations, const AnnotationList &)

  AnnotationList &GetAnnotations() { return m_Annotations; }

  void AddAnnotation(AbstractAnnotation *annot);

  void Reset();

  void SaveAnnotations(Registry &reg);
  void LoadAnnotations(Registry &reg);

protected:
  ImageAnnotationData() {}
  ~ImageAnnotationData() {}

  AnnotationList m_Annotations;
};

/** Iterator that searches for annotations */
template <class TAnnotPtr>
class ImageAnnotationIterator
{
public:
  ImageAnnotationIterator(const ImageAnnotationData *data);
  bool IsAtEnd() const;
  ImageAnnotationIterator &operator++();
  TAnnotPtr GetAnnotation() const;
  TAnnotPtr operator *() const { return this->GetAnnotation(); }
protected:
  typedef typename ImageAnnotationData::AnnotationConstIterator AnnotationConstIterator;
  AnnotationConstIterator m_Iter;
  const ImageAnnotationData *m_Data;
};

#endif // IMAGEANNOTATIONDATA_H
