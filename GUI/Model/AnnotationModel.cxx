#include "AnnotationModel.h"
#include "GenericSliceModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageAnnotationData.h"
#include "GlobalUIModel.h"
#include <limits>

void AnnotationModel::SetParent(GenericSliceModel *model)
{
  m_Parent = model;
  Rebroadcast(m_Parent->GetDriver()->GetGlobalState()->GetAnnotationModeModel(),
              ValueChangedEvent(), ModelUpdateEvent());
}

double AnnotationModel::GetCurrentLineLength()
{
  Vector2f pt1InAna = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.first);
  Vector2f pt2InAna = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.second);
  double length = (pt1InAna[0] - pt2InAna[0]) * (pt1InAna[0] - pt2InAna[0])
                + (pt1InAna[1] - pt2InAna[1]) * (pt1InAna[1] - pt2InAna[1]);
  length = sqrt(length);
  return length;
}

double AnnotationModel::GetCurrentLineLengthInPixels()
{
  Vector2f screen_offset =
      m_Parent->MapSliceToWindow(m_CurrentLine.first) -
      m_Parent->MapSliceToWindow(m_CurrentLine.second);

  return screen_offset.magnitude() / m_Parent->GetSizeReporter()->GetViewportPixelRatio();
}

double AnnotationModel::GetAngleWithCurrentLine(const annot::LineSegmentAnnotation *lsa)
{
  // Process the current line
  Vector2f v1 = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.first)
                - m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.second);
  v1 /= sqrt(v1[0]*v1[0] + v1[1]*v1[1]);

  // Process the annotation
  Vector2f v2 = m_Parent->MapSliceToPhysicalWindow(
                  m_Parent->MapImageToSlice(lsa->GetSegment().first))
                -
                m_Parent->MapSliceToPhysicalWindow(
                  m_Parent->MapImageToSlice(lsa->GetSegment().second));
  v2 /= sqrt(v2[0]*v2[0] + v2[1]*v2[1]);

  // Compute the dot product and no need for the third components that are zeros
  double angle = 180.0 * acos(fabs(v1[0]*v2[0]+v1[1]*v2[1])) / vnl_math::pi;
  return angle;
}

void AnnotationModel::AdjustAngleToRoundDegree(LineSegment &line, int n_degrees)
{
  ImageAnnotationData *adata = this->GetAnnotations();

  // Map the line segment from slice coordinates to window physical, where angles are
  // computed
  Vector2f p1 = m_Parent->MapSliceToPhysicalWindow(line.first);
  Vector2f p2 = m_Parent->MapSliceToPhysicalWindow(line.second);

  // Express the vector of the line in polar coordinates
  double p_rad = (p2 - p1).magnitude();
  double p_phase = atan2(p2[1] - p1[1], p2[0] - p1[0]) * 180.0 / vnl_math::pi;

  // Current proposed adjustment
  Vector2f p2_rot_best = p2;
  double rot_best = std::numeric_limits<double>::infinity();

  // Loop over all the lines
  for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    const annot::LineSegmentAnnotation *lsa =
        dynamic_cast<const annot::LineSegmentAnnotation *>(it->GetPointer());
    if(lsa && this->IsAnnotationVisible(lsa))
      {
      // Normalize the annotated line
      Vector2f q1 = m_Parent->MapSliceToPhysicalWindow(
                      m_Parent->MapImageToSlice(lsa->GetSegment().first));
      Vector2f q2 = m_Parent->MapSliceToPhysicalWindow(
                      m_Parent->MapImageToSlice(lsa->GetSegment().second));

      // Get the phase of the line
      double q_phase = atan2(q2[1] - q1[1], q2[0] - q1[0]) * 180.0 / vnl_math::pi;

      // Compute the updated phase - now the difference in phase is fractional
      double p_phase_round = q_phase + floor((p_phase - q_phase) / n_degrees + 0.5) * n_degrees;
      double p_phase_shift = fabs(p_phase_round - p_phase);

      // Map the rounded phase to the new p2 position
      Vector2f p2_prop(
            p1[0] + p_rad * cos(p_phase_round * vnl_math::pi / 180.0),
            p1[1] + p_rad * sin(p_phase_round * vnl_math::pi / 180.0));

      // Compare to current best
      if(p_phase_shift < rot_best)
        {
        rot_best = p_phase_shift;
        p2_rot_best = p2_prop;
        }
      }
    }

  // Map back
  line.second = m_Parent->MapPhysicalWindowToSlice(p2_rot_best);
}

Vector3f AnnotationModel::GetAnnotationCenter(const AbstractAnnotation *annot)
{
  const annot::LineSegmentAnnotation *lsa = dynamic_cast<const annot::LineSegmentAnnotation *>(annot);
  if(lsa)
    {
    return (m_Parent->MapImageToSlice(lsa->GetSegment().first)
            + m_Parent->MapImageToSlice(lsa->GetSegment().second)) * 0.5f;
    }


  return Vector3f(0.f);
}

double AnnotationModel::GetDistanceToLine(const Vector3f &x1, const Vector3f &x2, const Vector3d &point)
{
  Vector2f p0 = m_Parent->MapSliceToWindow(x1);
  Vector2f p1 = m_Parent->MapSliceToWindow(x2);
  Vector2f x = m_Parent->MapSliceToWindow(to_float(point));

  float alpha = dot_product(x - p0, p1 - p0) / dot_product(p1 - p0, p1 - p0);
  if(alpha < 0)
    alpha = 0;
  if(alpha > 1)
    alpha = 1;

  Vector2f px = p0 * (1.0f - alpha) + p1 * alpha;
  return (px - x).magnitude();

}


double AnnotationModel::GetDistanceToLine(LineSegment &line, const Vector3d &point)
{
  return GetDistanceToLine(line.first, line.second, point);
}

AnnotationMode AnnotationModel::GetAnnotationMode() const
{
  return m_Parent->GetDriver()->GetGlobalState()->GetAnnotationMode();
}

ImageAnnotationData *AnnotationModel::GetAnnotations()
{
  return m_Parent->GetDriver()->GetCurrentImageData()->GetAnnotations();
}

bool AnnotationModel::IsAnnotationVisible(const AnnotationModel::AbstractAnnotation *annot)
{
  return annot->IsVisible(
        m_Parent->GetSliceDirectionInImageSpace(),
        m_Parent->GetSliceIndex());
}

double AnnotationModel
::GetPixelDistanceToAnnotation(
    const AbstractAnnotation *annot,
    const Vector3d &point)
{
  const annot::LineSegmentAnnotation *lsa = dynamic_cast<const annot::LineSegmentAnnotation *>(annot);
  if(lsa)
    {
    const annot::LineSegment &seg = lsa->GetSegment();
    Vector3f s1 = m_Parent->MapImageToSlice(seg.first);
    Vector3f s2 = m_Parent->MapImageToSlice(seg.second);
    return GetDistanceToLine(s1, s2, point);
    }

  const annot::LandmarkAnnotation *lma = dynamic_cast<const annot::LandmarkAnnotation *>(annot);
  if(lma)
    {
    const annot::Landmark &landmark = lma->GetLandmark();
    Vector3f s1, s2;
    this->GetLandmarkArrowPoints(landmark, s1, s2);
    return GetDistanceToLine(s1, s2, point);
    }

  return std::numeric_limits<double>::infinity();
}

AnnotationModel::AbstractAnnotation *
AnnotationModel::GetAnnotationUnderCursor(const Vector3d &xSlice)
{
  ImageAnnotationData *adata = this->GetAnnotations();

  // Current best annotation
  AbstractAnnotation *asel = NULL;
  double dist_min = std::numeric_limits<double>::infinity();
  double dist_thresh = 5 * m_Parent->GetSizeReporter()->GetViewportPixelRatio();

  // Loop over all annotations
  for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    AbstractAnnotation *a = *it;

    // Test if annotation is visible in this plane
    if(this->IsAnnotationVisible(a))
      {
      double dist = GetPixelDistanceToAnnotation(a, xSlice);
      if(dist < dist_thresh && dist < dist_min)
        {
        asel = a;
        dist_min = dist;
        }
      }
    }

  return asel;
}

bool AnnotationModel::ProcessPushEvent(const Vector3d &xSlice, bool shift_mod)
{
  // Get the annotation data
  ImageAnnotationData *adata = this->GetAnnotations();

  bool handled = false;
  if(this->GetAnnotationMode() == ANNOTATION_RULER || this->GetAnnotationMode() == ANNOTATION_LANDMARK)
    {
    if(m_FlagDrawingLine)
      {
      // Complete drawing line
      m_CurrentLine.second = to_float(xSlice);
      handled = true;
      }
    else
      {
      m_CurrentLine.first = to_float(xSlice);
      m_CurrentLine.second = to_float(xSlice);
      m_FlagDrawingLine = true;
      handled = true;
      }
    }

  else if(this->GetAnnotationMode() == ANNOTATION_SELECT)
    {
    // Current best annotation
    AbstractAnnotation *asel = this->GetAnnotationUnderCursor(xSlice);

    // If the shift modifier is on, we add to the selection
    if(shift_mod)
      {
      // Add to the selection
      if(asel)
        asel->SetSelected(!asel->GetSelected());
      }
    else
      {
      // Clear the selection
      for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
          it != adata->GetAnnotations().end(); ++it)
        (*it)->SetSelected(false);

      // Select the clicked
      if(asel)
        asel->SetSelected(!asel->GetSelected());
      }

    // Store the position of the drag-start
    if(asel)
      {
      m_MovingSelection = true;
      m_DragStart = to_float(xSlice);
      m_DragLast = to_float(xSlice);
      }
    else
      {
      m_MovingSelection = false;
      }

    // We always consider the click as handled because otherwise it can be really annoying
    // for the user if they miss an annotation and that results in the cross-hairs being
    // moved to another location
    handled = true;
    }

  if(handled)
    InvokeEvent(ModelUpdateEvent());
  return handled;
}

bool AnnotationModel::ProcessMoveEvent(const Vector3d &xSlice, bool shift_mod, bool drag)
{
  ImageAnnotationData *adata = this->GetAnnotations();
  bool handled = false;
  if(this->GetAnnotationMode() == ANNOTATION_RULER || this->GetAnnotationMode() == ANNOTATION_LANDMARK)
    {
    if(m_FlagDrawingLine)
      {
      // Accept the second point
      m_CurrentLine.second = to_float(xSlice);

      // If shift pressed, adjust the line to have a integral angle with one of existing
      // lines
      if(this->GetAnnotationMode() == ANNOTATION_RULER  && shift_mod)
        {
        this->AdjustAngleToRoundDegree(m_CurrentLine, 5);
        }

      handled = true;
      }
    }
  else if(this->GetAnnotationMode() == ANNOTATION_SELECT && m_MovingSelection && drag)
    {

    // Compute the amount to move the annotation by
    Vector3f p_last = m_Parent->MapSliceToImage(m_DragLast);
    Vector3f p_now = m_Parent->MapSliceToImage(to_float(xSlice));
    Vector3f p_delta = p_now - p_last;

    // Move the currently selected annotations
    for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
        it != adata->GetAnnotations().end(); ++it)
      {
      AbstractAnnotation *a = *it;

      // Test if annotation is visible in this plane
      if(a->GetSelected() && this->IsAnnotationVisible(a))
        {
        // Move the annotation by this amount
        a->MoveBy(p_delta);
        }
      }

    // Store the update point
    m_DragLast = to_float(xSlice);

    // Event has been handled
    handled = true;
    }

  if(handled)
    this->InvokeEvent(ModelUpdateEvent());
  return handled;
}

bool AnnotationModel::ProcessReleaseEvent(const Vector3d &xSlice, bool shift_mod)
{
  // Handle as a drag event
  bool handled = this->ProcessMoveEvent(xSlice, shift_mod, true);

  // If drawing line, complete the line drawing, as long as the line is long enough
  if(m_FlagDrawingLine)
    {
    // If line is longer than a threshold of 5 pixel units, mark it as completed
    if(this->GetCurrentLineLengthInPixels() > 5)
      m_FlagDrawingLine = false;
    }

  return handled;
}

bool AnnotationModel::IsDrawingRuler()
{
  return this->GetAnnotationMode() == ANNOTATION_RULER && m_FlagDrawingLine;
}

void AnnotationModel::AcceptLine()
{
  // Get the length of the line in logical (non-retina) pixels
  // Check that the length of the segment is at least 5 screen pixels


  if(this->GetAnnotationMode() == ANNOTATION_RULER)
    {
    // Create the line in image space
    annot::LineSegment ls = std::make_pair(
                              m_Parent->MapSliceToImage(m_CurrentLine.first),
                              m_Parent->MapSliceToImage(m_CurrentLine.second));
    // Add the line
    SmartPtr<annot::LineSegmentAnnotation> lsa = annot::LineSegmentAnnotation::New();
    lsa->SetSegment(ls);
    lsa->SetPlane(m_Parent->GetSliceDirectionInImageSpace());
    lsa->SetVisibleInAllPlanes(false);
    lsa->SetVisibleInAllSlices(false);
    lsa->SetColor(m_Parent->GetParentUI()->GetGlobalState()->GetAnnotationColor());
    lsa->SetSelected(false);

    this->GetAnnotations()->AddAnnotation(lsa);
    m_FlagDrawingLine = false;

    this->InvokeEvent(ModelUpdateEvent());
    }

  else if(this->GetAnnotationMode() == ANNOTATION_LANDMARK)
    {
    // Create the line in image space
    annot::Landmark lm;
    lm.Text = this->GetCurrentAnnotationText();
    lm.Pos = m_Parent->MapSliceToImage(m_CurrentLine.first);

    // Set the default offset. The default offset corresponds to 5 screen pixels
    // to the top right. We need to map this into image units
    Vector2f xHeadWin = m_Parent->MapSliceToWindow(m_CurrentLine.first);
    Vector2f xTailWin = m_Parent->MapSliceToWindow(m_CurrentLine.second);
    Vector2f xHeadWinPhys = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.first);
    Vector2f xTailWinPhys = m_Parent->MapSliceToPhysicalWindow(m_Parent->MapWindowToSlice(xTailWin));
    lm.Offset = xTailWinPhys - xHeadWinPhys;

    // Add the line
    SmartPtr<annot::LandmarkAnnotation> lma = annot::LandmarkAnnotation::New();
    lma->SetLandmark(lm);
    lma->SetPlane(m_Parent->GetSliceDirectionInImageSpace());
    lma->SetVisibleInAllPlanes(false);
    lma->SetVisibleInAllSlices(false);
    lma->SetColor(m_Parent->GetParentUI()->GetGlobalState()->GetAnnotationColor());
    lma->SetSelected(false);

    this->GetAnnotations()->AddAnnotation(lma);

    this->InvokeEvent(ModelUpdateEvent());
    }
}

void AnnotationModel::CancelLine()
{
  m_FlagDrawingLine = false;
  this->InvokeEvent(ModelUpdateEvent());
}

void AnnotationModel::SelectAllOnSlice()
{
  ImageAnnotationData *adata = this->GetAnnotations();
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    AbstractAnnotation *a = *it;

    // Test if annotation is visible in this plane
    if(this->IsAnnotationVisible(a))
      {
      a->SetSelected(true);
      }
    }

  this->InvokeEvent(ModelUpdateEvent());
}

void AnnotationModel::DeleteSelectedOnSlice()
{
  ImageAnnotationData *adata = this->GetAnnotations();
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); )
    {
    AbstractAnnotation *a = *it;

    // Test if annotation is visible in this plane
    if(a->GetSelected() && this->IsAnnotationVisible(a))
      {
      it = adata->GetAnnotations().erase(it);
      }
    else
      ++it;
    }

  this->InvokeEvent(ModelUpdateEvent());
}

void AnnotationModel::GoToNextAnnotation()
{
  this->GoToNextOrPrevAnnotation(1);
}

void AnnotationModel::GoToPreviousAnnotation()
{
  this->GoToNextOrPrevAnnotation(-1);
}

void AnnotationModel::GoToNextOrPrevAnnotation(int direction)
{
  // Create a list of all annotations in this slice view, sorted
  typedef std::pair<long, AbstractAnnotation *> AnnotPair;
  std::list<AnnotPair> annot_list;
  typedef std::list<AnnotPair>::iterator AnnotIter;
  typedef std::list<AnnotPair>::reverse_iterator AnnotRevIter;

  // Find annotation that will serve as a reference point
  AbstractAnnotation *ref_annot = NULL;
  AbstractAnnotation *selected = NULL;

  // Iterate through the annotations
  ImageAnnotationData *adata = this->GetAnnotations();
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    AbstractAnnotation *a = *it;
    if(a->IsVisible(m_Parent->GetSliceDirectionInImageSpace()))
      {
      // Create a pair for the current annotation
      Vector3f ank_img = a->GetAnchorPoint(m_Parent->GetSliceDirectionInImageSpace());
      Vector3f ank_slice = m_Parent->MapImageToSlice(ank_img);

      long hash = ank_slice[2] * 100000000l + ank_slice[1] * 10000l + ank_slice[0];

      AnnotPair pair = std::make_pair(hash, a);
      annot_list.push_back(pair);

      // If the annotation is on this slice and selected, us as a reference
      if(this->IsAnnotationVisible(a) && a->GetSelected())
        ref_annot = a;
      }
    }

  // Test for degenerate cases
  if(annot_list.size() == 1)
    {
    selected = annot_list.front().second;
    }
  else if(annot_list.size() > 1)
    {
    // Sort the annotations by slice, then by in-slice position
    annot_list.sort();

    // Find the reference annotation if it exists
    if(ref_annot && direction > 0)
      {
      AnnotIter it_ref = annot_list.end();
      for(AnnotIter it = annot_list.begin(); it!=annot_list.end(); ++it)
        if(it->second == ref_annot)
          it_ref = it;

      ++it_ref;
      if(it_ref == annot_list.end())
        it_ref = annot_list.begin();

      selected = it_ref->second;
      }
    else if(ref_annot && direction < 0)
      {
      AnnotRevIter it_ref = annot_list.rend();
      for(AnnotRevIter it = annot_list.rbegin(); it!=annot_list.rend(); ++it)
        if(it->second == ref_annot)
          it_ref = it;

      ++it_ref;
      if(it_ref == annot_list.rend())
        it_ref = annot_list.rbegin();

      selected = it_ref->second;
      }
    else if(direction > 0)
      {
      // Start from the beginning and find something on the current slice or after it
      for(AnnotIter it = annot_list.begin(); it!=annot_list.end(); ++it)
        if(it->second->GetSliceIndex(m_Parent->GetSliceDirectionInImageSpace()) >=
           m_Parent->GetSliceIndex())
          {
          selected = it->second;
          break;
          }

      if(!selected)
        selected = annot_list.front().second;
      }
    else
      {
      // Start from the beginning and find something on the current slice or after it
      for(AnnotRevIter it = annot_list.rbegin(); it!=annot_list.rend(); --it)
        if(it->second->GetSliceIndex(m_Parent->GetSliceDirectionInImageSpace()) <=
           m_Parent->GetSliceIndex())
          {
          selected = it->second;
          break;
          }

      if(!selected)
        selected = annot_list.back().second;
      }
    }

  // Deselect everything
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    (*it)->SetSelected(false);
    }

  // Select the clicked
  if(selected)
    {
    // Select the next item
    selected->SetSelected(true);

    // Go to its plane
    Vector3ui cursor = m_Parent->GetDriver()->GetCursorPosition();
    cursor[m_Parent->GetSliceDirectionInImageSpace()] = selected->GetSliceIndex(m_Parent->GetSliceDirectionInImageSpace());
    m_Parent->GetDriver()->SetCursorPosition(cursor);
    }

  // Fire event
  this->InvokeEvent(ModelUpdateEvent());
}

bool AnnotationModel::CheckState(AnnotationModel::UIState state)
{
  switch(state)
    {
    case AnnotationModel::UIF_LINE_MODE:
      return this->GetAnnotationMode() == ANNOTATION_RULER;
      break;
    case AnnotationModel::UIF_LANDMARK_MODE:
      return this->GetAnnotationMode() == ANNOTATION_LANDMARK;
      break;
    case AnnotationModel::UIF_LINE_MODE_DRAWING:
      // return this->IsDrawingRuler();
      return this->GetFlagDrawingLine();
      break;
    case AnnotationModel::UIF_EDITING_MODE:
      return this->GetAnnotationMode() == ANNOTATION_SELECT;
      break;

    }

  return false;
}

bool AnnotationModel::IsHoveringOverAnnotation(const Vector3d &xSlice)
{
  return (this->GetAnnotationUnderCursor(xSlice) != NULL);
}

void AnnotationModel::GetLandmarkArrowPoints(const annot::Landmark &lm,
                                             Vector3f &outHeadXSlice, Vector3f &outTailXSlice)
{
  // Get the head coordinates in slice units
  outHeadXSlice = m_Parent->MapImageToSlice(lm.Pos);

  // Get the tail coordinate in slice units
  outTailXSlice = m_Parent->MapPhysicalWindowToSlice(
        m_Parent->MapSliceToPhysicalWindow(outHeadXSlice) + lm.Offset);
}

AnnotationModel::AnnotationModel()
{
  m_FlagDrawingLine = false;
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

AnnotationModel::~AnnotationModel()
{

}
