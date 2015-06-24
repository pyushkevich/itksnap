#include "ImageAnnotationData.h"

namespace annot
{

bool AbstractAnnotation::IsVisible(int plane, int slice) const
{
  // Check if the plane makes this annotation invisible
  if(!m_VisibleInAllPlanes && plane != m_Plane)
    return false;

  // Check if the slice makes this annotation invisible
  if(!m_VisibleInAllSlices && slice != this->GetSliceIndex(plane))
    return false;

  return true;
}

int LineSegmentAnnotation::GetSliceIndex(int plane) const
{
  // Just return the coordinate of the first point. It should be the same
  // as the coordinate of the second point
  assert(plane == m_Plane);
  assert(m_Segment.first[plane] == m_Segment.second[plane]);

  return (int) (m_Segment.first[plane]);
}

void LineSegmentAnnotation::MoveBy(const Vector3f &offset)
{
  m_Segment.first += offset;
  m_Segment.second += offset;
}

int LandmarkAnnotation::GetSliceIndex(int plane) const
{
  // Just return the coordinate of the first point. It should be the same
  // as the coordinate of the second point
  return (int) (m_Landmark.Pos[plane]);
}

void LandmarkAnnotation::MoveBy(const Vector3f &offset)
{
  m_Landmark.Pos += offset;
}


}

void ImageAnnotationData::AddAnnotation(ImageAnnotationData::AbstractAnnotation *annot)
{
  SmartPtr<AbstractAnnotation> myannot = annot;
  m_Annotations.push_back(myannot);
}

void ImageAnnotationData::Reset()
{
  m_Annotations.clear();
}


