#include "ImageAnnotationData.h"
#include "Registry.h"
#include "IRISException.h"

namespace annot
{

/** Global annotation Id counter */
unsigned long GlobalAnnotationIndex = 0;

Vector3ui AbstractAnnotation::GetColor3ui() const
{
  return to_unsigned_int(this->GetColor() * 255.0 + 0.5);
}

void AbstractAnnotation::SetColor3ui(const Vector3ui &color)
{
  this->SetColor(to_double(color) / 255.0);
}

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

bool AbstractAnnotation::IsVisible(int plane) const
{
  // Check if the plane makes this annotation invisible
  if(!m_VisibleInAllPlanes && plane != m_Plane)
    return false;

  return true;
}

void AbstractAnnotation::Save(Registry &folder)
{
  folder["Selected"] << m_Selected;
  folder["VisibleInAllSlices"] << m_VisibleInAllSlices;
  folder["VisibleInAllPlanes"] << m_VisibleInAllPlanes;
  folder["Plane"] << m_Plane;
  folder["Color"] << m_Color;
  folder["Tags"].PutList(m_Tags);
}

void AbstractAnnotation::Load(Registry &folder)
{
  m_Selected = folder["Selected"][false];
  m_VisibleInAllSlices = folder["VisibleInAllSlices"][false];
  m_VisibleInAllPlanes = folder["VisibleInAllPlanes"][false];
  m_Plane = folder["Plane"][0];
  m_Color = folder["Color"][Vector3d(1.0, 0.0, 0.0)];
  folder["Tags"].GetList(m_Tags);
}

unsigned long AbstractAnnotation::GetUniqueId() const
{
  return m_UniqueId;
}

AbstractAnnotation::AbstractAnnotation()
{
  m_UniqueId = ++GlobalAnnotationIndex;
}

int LineSegmentAnnotation::GetSliceIndex(int plane) const
{
  // Just return the coordinate of the first point. It should be the same
  // as the coordinate of the second point
  assert(plane == m_Plane);
  assert(m_Segment.first[plane] == m_Segment.second[plane]);

  return (int) (m_Segment.first[plane]);
}

Vector3d LineSegmentAnnotation::GetAnchorPoint(int plane) const
{
  // Use the midpoint
  return (m_Segment.first + m_Segment.second) * 0.5;
}

void LineSegmentAnnotation::Save(Registry &folder)
{
  Superclass::Save(folder);
  folder["Type"] << "LineSegmentAnnotation";
  folder["Point1"] << to_double(m_Segment.first);
  folder["Point2"] << to_double(m_Segment.second);
}

void LineSegmentAnnotation::Load(Registry &folder)
{
  Superclass::Load(folder);
  m_Segment.first = folder["Point1"][Vector3d(0.0)];
  m_Segment.second = folder["Point2"][Vector3d(0.0)];
  if(m_Segment.first[this->m_Plane] != m_Segment.second[this->m_Plane])
    throw IRISException("Invalid line segment annotation detected in file.");
}

void LineSegmentAnnotation::MoveBy(const Vector3d &offset)
{
  m_Segment.first += offset;
  m_Segment.second += offset;
}

Vector3d LineSegmentAnnotation::GetCenter() const
{
  return (m_Segment.first + m_Segment.second) / 2.0;
}

int LandmarkAnnotation::GetSliceIndex(int plane) const
{
  // Just return the coordinate of the first point. It should be the same
  // as the coordinate of the second point
  return (int) (m_Landmark.Pos[plane]);
}

Vector3d LandmarkAnnotation::GetAnchorPoint(int plane) const
{
  return m_Landmark.Pos;
}

void LandmarkAnnotation::MoveBy(const Vector3d &offset)
{
  m_Landmark.Pos += offset;
}

Vector3d LandmarkAnnotation::GetCenter() const
{
  return m_Landmark.Pos;
}

void LandmarkAnnotation::Save(Registry &folder)
{
  Superclass::Save(folder);
  folder["Type"] << "LandmarkAnnotation";
  folder["Pos"] << to_double(m_Landmark.Pos);
  folder["Offset"] << to_double(m_Landmark.Offset);
  folder["Text"] << m_Landmark.Text;
}

void LandmarkAnnotation::Load(Registry &folder)
{
  Superclass::Load(folder);
  m_Landmark.Pos = folder["Pos"][Vector3d(0.0)];
  m_Landmark.Offset = folder["Offset"][Vector2d(0.0)];
  m_Landmark.Text = folder["Text"]["??? Landmark"];
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

void ImageAnnotationData::SaveAnnotations(Registry &reg)
{
  // Store the current format of the annotations
  reg["Format"] << "ITK-SNAP Annotation File";

  // Format date specifies the date this format was developed. If in the future
  // the format changes in drastic ways, this allows the future code to recover.
  reg["FormatDate"] << "20150624";

  // Save the array of annotations
  reg["Annotations.ArraySize"] << m_Annotations.size();
  int i = 0;
  for(AnnotationConstIterator it = m_Annotations.begin(); it != m_Annotations.end(); it++, i++)
    {
    AbstractAnnotation *ann = *it;
    Registry &folder = reg.Folder(reg.Key("Annotations.Element[%d]", i));
    ann->Save(folder);
    }
}


void ImageAnnotationData::LoadAnnotations(Registry &reg)
{
  // Read the format
  std::string format = reg["Format"][""];
  std::string format_date = reg["FormatDate"][""];
  if(format != "ITK-SNAP Annotation File" || format_date.length() != 8)
    throw IRISException("Annotation file is not in the correct format.");

  // Clear the annotations
  m_Annotations.clear();

  // Read the list of annotations
  int n_annot = reg["Annotations.ArraySize"][0];
  for(int i = 0; i < n_annot; i++)
    {
    Registry &folder = reg.Folder(reg.Key("Annotations.Element[%d]", i));

    // Factory code
    std::string type = folder["Type"][""];
    AnnotationPtr ann;
    if(type == "LineSegmentAnnotation")
      {
      SmartPtr<annot::LineSegmentAnnotation> line_ann =  annot::LineSegmentAnnotation::New();
      ann = line_ann.GetPointer();
      }
    else if(type == "LandmarkAnnotation")
      {
      SmartPtr<annot::LandmarkAnnotation> lm_ann =  annot::LandmarkAnnotation::New();
      ann = lm_ann.GetPointer();
      }

    // Read the annotation
    if(ann)
      {
      ann->Load(folder);
      m_Annotations.push_back(ann);
      }
    }
}

template<class TAnnotPtr>
ImageAnnotationIterator<TAnnotPtr>
::ImageAnnotationIterator(const ImageAnnotationData *data)
  : m_Data(data)
{
  // Search for the first annotation of desired type
  m_Iter = m_Data->GetAnnotations().begin();
  while(!IsAtEnd())
    {
    TAnnotPtr ann = dynamic_cast<TAnnotPtr>(m_Iter->GetPointer());
    if(ann)
      break;
    else
      ++m_Iter;
    }
}

template<class TAnnotPtr>
bool
ImageAnnotationIterator<TAnnotPtr>
::IsAtEnd() const
{
  return m_Iter == m_Data->GetAnnotations().end();
}

template<class TAnnotPtr>
ImageAnnotationIterator<TAnnotPtr> &
ImageAnnotationIterator<TAnnotPtr>
::operator++()
{
  if(!IsAtEnd())
    ++m_Iter;

  while(!IsAtEnd())
    {
    TAnnotPtr ann = dynamic_cast<TAnnotPtr>(m_Iter->GetPointer());
    if(ann)
      break;
    else
      ++m_Iter;
    }

  return *this;
}

template<class TAnnotPtr>
TAnnotPtr
ImageAnnotationIterator<TAnnotPtr>
::GetAnnotation() const
{
  if(!this->IsAtEnd())
    return dynamic_cast<TAnnotPtr>(m_Iter->GetPointer());

  return NULL;
}


template class ImageAnnotationIterator<annot::LandmarkAnnotation *>;
template class ImageAnnotationIterator<annot::LineSegmentAnnotation *>;
template class ImageAnnotationIterator<annot::AbstractAnnotation *>;
