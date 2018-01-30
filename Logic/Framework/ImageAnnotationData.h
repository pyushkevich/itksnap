#ifndef IMAGEANNOTATIONDATA_H
#define IMAGEANNOTATIONDATA_H

#include "SNAPCommon.h"
#include <utility>
#include <string>
#include <list>
#include "itkDataObject.h"
#include "itkObjectFactory.h"

class Registry;

namespace annot
{

typedef Vector3f Point;

/**
 * @brief Slices where the annotation is displayed
 */
enum VisSlice {
  SINGLE_SLICE = 0, ALL_SLICES
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

  /** Test whether the annotation is visible in the current plane and given slice */
  bool IsVisible(int plane, int slice) const;

  /** Test whether the annotation is visible in the current plane in some slice */
  bool IsVisible(int plane) const;

  /** Get the slice this annotation belongs to for a particular plane */
  virtual int GetSliceIndex(int plane) const = 0;

  /** Get the anchor point for the annotation in a given plane - used for sorting */
  virtual Vector3f GetAnchorPoint(int plane) const = 0;

  /** Move the annotation by given amount in physical space */
  virtual void MoveBy(const Vector3f &offset) = 0;

  /** Save the annotation data to registry */
  virtual void Save(Registry &folder);

  /** Load from the registry */
  virtual void Load(Registry &folder);

protected:

  AbstractAnnotation() {}
  ~AbstractAnnotation() {}

  bool m_Selected;
  bool m_VisibleInAllSlices;
  bool m_VisibleInAllPlanes;

  Vector3d m_Color;

  int m_Plane;
};

/** A simple line segment */
typedef std::pair<Vector3f,Vector3f>           LineSegment;

class LineSegmentAnnotation : public AbstractAnnotation
{
public:
  irisITKObjectMacro(LineSegmentAnnotation, AbstractAnnotation)

  typedef LineSegment                   ObjectType;

  irisGetSetMacro(Segment, const LineSegment &)

  virtual void Save(Registry &folder);
  virtual void Load(Registry &folder);

  virtual void MoveBy(const Vector3f &offset);

protected:

  virtual int GetSliceIndex(int plane) const;

  virtual Vector3f GetAnchorPoint(int plane) const;

  LineSegment m_Segment;
};

/**
 * @brief Text with location
 */
struct Landmark
{
  std::string Text;
  Vector3f Pos;
  Vector2f Offset;
};

class LandmarkAnnotation : public AbstractAnnotation
{
public:
  irisITKObjectMacro(LandmarkAnnotation, AbstractAnnotation)

  typedef Landmark                   ObjectType;

  irisGetSetMacro(Landmark, const Landmark &)


protected:

  virtual int GetSliceIndex(int plane) const;
  virtual Vector3f GetAnchorPoint(int plane) const;

  virtual void MoveBy(const Vector3f &offset);

  virtual void Save(Registry &folder);
  virtual void Load(Registry &folder);

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

#endif // IMAGEANNOTATIONDATA_H
