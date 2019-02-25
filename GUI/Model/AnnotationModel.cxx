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

double AnnotationModel::GetLineLength(const Vector3d &xSliceA, const Vector3d &xSliceB)
{
  Vector2d pt1InAna = m_Parent->MapSliceToPhysicalWindow(xSliceA);
  Vector2d pt2InAna = m_Parent->MapSliceToPhysicalWindow(xSliceB);
  double length = (pt1InAna[0] - pt2InAna[0]) * (pt1InAna[0] - pt2InAna[0])
                + (pt1InAna[1] - pt2InAna[1]) * (pt1InAna[1] - pt2InAna[1]);
  length = sqrt(length);
  return length;
}

double AnnotationModel::GetCurrentLineLength()
{
  return GetLineLength(m_CurrentLine.first, m_CurrentLine.second);
}

double AnnotationModel::GetCurrentLineLengthInPixels()
{
  Vector2d screen_offset =
      m_Parent->MapSliceToWindow(m_CurrentLine.first) -
      m_Parent->MapSliceToWindow(m_CurrentLine.second);

  return screen_offset.magnitude() / m_Parent->GetSizeReporter()->GetViewportPixelRatio();
}

double AnnotationModel::GetAngleWithCurrentLine(const annot::LineSegmentAnnotation *lsa)
{
  // Process the current line
  Vector2d v1 = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.first)
                - m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.second);
  v1 /= sqrt(v1[0]*v1[0] + v1[1]*v1[1]);

  // Process the annotation
  Vector2d v2 = m_Parent->MapSliceToPhysicalWindow(
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
  Vector2d p1 = m_Parent->MapSliceToPhysicalWindow(line.first);
  Vector2d p2 = m_Parent->MapSliceToPhysicalWindow(line.second);

  // Express the vector of the line in polar coordinates
  double p_rad = (p2 - p1).magnitude();
  double p_phase = atan2(p2[1] - p1[1], p2[0] - p1[0]) * 180.0 / vnl_math::pi;

  // Current proposed adjustment
  Vector2d p2_rot_best = p2;
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
      Vector2d q1 = m_Parent->MapSliceToPhysicalWindow(
                      m_Parent->MapImageToSlice(lsa->GetSegment().first));
      Vector2d q2 = m_Parent->MapSliceToPhysicalWindow(
                      m_Parent->MapImageToSlice(lsa->GetSegment().second));

      // Get the phase of the line
      double q_phase = atan2(q2[1] - q1[1], q2[0] - q1[0]) * 180.0 / vnl_math::pi;

      // Compute the updated phase - now the difference in phase is fractional
      double p_phase_round = q_phase + floor((p_phase - q_phase) / n_degrees + 0.5) * n_degrees;
      double p_phase_shift = fabs(p_phase_round - p_phase);

      // Map the rounded phase to the new p2 position
      Vector2d p2_prop(
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

Vector3d AnnotationModel::GetAnnotationCenter(const AbstractAnnotation *annot)
{
  const annot::LineSegmentAnnotation *lsa = dynamic_cast<const annot::LineSegmentAnnotation *>(annot);
  if(lsa)
    {
    return (m_Parent->MapImageToSlice(lsa->GetSegment().first)
            + m_Parent->MapImageToSlice(lsa->GetSegment().second)) * 0.5;
    }


  return Vector3d(0.0);
}

double AnnotationModel::GetDistanceToLine(const Vector3d &x1, const Vector3d &x2, const Vector3d &point)
{
  Vector2d p0 = m_Parent->MapSliceToWindow(x1);
  Vector2d p1 = m_Parent->MapSliceToWindow(x2);
  Vector2d x = m_Parent->MapSliceToWindow(point);

  double alpha = dot_product(x - p0, p1 - p0) / dot_product(p1 - p0, p1 - p0);
  if(alpha < 0)
    alpha = 0;
  if(alpha > 1)
    alpha = 1;

  Vector2d px = p0 * (1.0f - alpha) + p1 * alpha;
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

bool AnnotationModel::IsAnnotationModeActive() const
{
  GlobalState *gs = this->GetParent()->GetParentUI()->GetGlobalState();
  return gs->GetToolbarMode() == ANNOTATION_MODE;
}

ImageAnnotationData *AnnotationModel::GetAnnotations() const
{
  return m_Parent->GetDriver()->GetCurrentImageData()->GetAnnotations();
}

bool AnnotationModel::IsAnnotationVisible(const AnnotationModel::AbstractAnnotation *annot) const
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
    Vector3d s1 = m_Parent->MapImageToSlice(seg.first);
    Vector3d s2 = m_Parent->MapImageToSlice(seg.second);
    return GetDistanceToLine(s1, s2, point);
    }

  const annot::LandmarkAnnotation *lma = dynamic_cast<const annot::LandmarkAnnotation *>(annot);
  if(lma)
    {
    const annot::Landmark &landmark = lma->GetLandmark();
    Vector3d s1, s2;
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
      m_CurrentLine.second = xSlice;
      handled = true;
      }
    else
      {
      m_CurrentLine.first = xSlice;
      m_CurrentLine.second = xSlice;
      m_FlagDrawingLine = true;
      handled = true;
      }
    }

  else if(this->GetAnnotationMode() == ANNOTATION_SELECT)
    {
    // Check if for any of the selected annotations, the click is close to the drag handle
    int handle_idx = -1;
    AbstractAnnotation *asel = this->GetSelectedHandleUnderCusror(xSlice, handle_idx);

    // Find closest annotation under the cursor
    if(!asel)
      asel = this->GetAnnotationUnderCursor(xSlice);

    // If the shift modifier is on, we add to the selection
    if(shift_mod)
      {
      // Add to the selection
      if(asel)
        asel->SetSelected(!asel->GetSelected());
      }
    // If not clicked on a selected handle, process as a select operation
    else if(handle_idx < 0)
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
      m_DragStart = xSlice;
      m_DragLast = xSlice;
      m_MovingSelectionHandle = handle_idx;
      m_MovingSelectionHandleAnnot = asel;
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
      m_CurrentLine.second = xSlice;

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
    Vector3d p_last = m_Parent->MapSliceToImage(m_DragLast);
    Vector3d p_now = m_Parent->MapSliceToImage(xSlice);
    Vector3d p_delta = p_now - p_last;

    // Process the move command on selected annotations
    for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
        it != adata->GetAnnotations().end(); ++it)
      {
      AbstractAnnotation *a = *it;

      // Test if annotation is visible in this plane
      if(m_MovingSelectionHandle < 0 && a->GetSelected() && this->IsAnnotationVisible(a))
        {
        // Move the annotation by this amount
        a->MoveBy(p_delta);
        }
      else if(m_MovingSelectionHandle >= 0 && m_MovingSelectionHandleAnnot == a)
        {
        // Move the annotation handle by this amount
        this->MoveAnnotationHandle(a, m_MovingSelectionHandle, p_delta);
        }
      }

    // Store the update point
    m_DragLast = xSlice;

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
    Vector2d xHeadWin = m_Parent->MapSliceToWindow(m_CurrentLine.first);
    Vector2d xTailWin = m_Parent->MapSliceToWindow(m_CurrentLine.second);
    Vector2d xHeadWinPhys = m_Parent->MapSliceToPhysicalWindow(m_CurrentLine.first);
    Vector2d xTailWinPhys = m_Parent->MapSliceToPhysicalWindow(m_Parent->MapWindowToSlice(xTailWin));
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

AnnotationModel::AbstractAnnotation *
AnnotationModel::GetSingleSelectedAnnotation() const
{
  ImageAnnotationData *adata = this->GetAnnotations();
  AbstractAnnotation *last_sel = NULL;
  unsigned int n_found = 0;
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); it++)
    {
    AbstractAnnotation *a = *it;
    if(a->GetSelected() && this->IsAnnotationVisible(a))
      {
      n_found++;
      last_sel = a;
      }
    }

  return n_found == 1 ? last_sel : NULL;
}

unsigned int
AnnotationModel::GetAnnotationCount(bool filter_selected, bool filter_visible) const
{
  ImageAnnotationData *adata = this->GetAnnotations();
  unsigned int n_found = 0;
  for(ImageAnnotationData::AnnotationIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); it++)
    {
    AbstractAnnotation *a = *it;
    if(a->GetPlane() == m_Parent->GetSliceDirectionInImageSpace()
       && (!filter_selected || a->GetSelected())
       && (!filter_visible || this->IsAnnotationVisible(a)))
      {
      n_found++;
      }
    }

  return n_found;
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
      Vector3d ank_img = a->GetAnchorPoint(m_Parent->GetSliceDirectionInImageSpace());
      Vector3d ank_slice = m_Parent->MapImageToSlice(ank_img);

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

bool AnnotationModel::TestPointInClickRadius(const Vector3d &xClickSlice,
                                             const Vector3d &xPointSlice,
                                             int logical_pixels)
{
  Vector2d clickW = m_Parent->MapSliceToWindow(xClickSlice);
  Vector2d pointW = m_Parent->MapSliceToWindow(xPointSlice);
  int vppr = m_Parent->GetSizeReporter()->GetViewportPixelRatio();

  return
      fabs(clickW[0] - pointW[0]) <= logical_pixels * vppr
      &&
      fabs(clickW[1] - pointW[1]) <= logical_pixels * vppr;
}

void AnnotationModel::MoveAnnotationHandle(AnnotationModel::AbstractAnnotation *ann, int handle, const Vector3d &deltaPhys)
{
  // Draw all the line segments
  annot::LineSegmentAnnotation *lsa =
      dynamic_cast<annot::LineSegmentAnnotation *>(ann);
  if(lsa)
    {
    annot::LineSegment ls = lsa->GetSegment();
    if(handle == 0)
      ls.first += deltaPhys;
    else if(handle == 1)
      ls.second += deltaPhys;
    lsa->SetSegment(ls);
    }

  // Draw all the line segments
  annot::LandmarkAnnotation *lma =
      dynamic_cast<annot::LandmarkAnnotation *>(ann);
  if(lma)
    {
    annot::Landmark lm = lma->GetLandmark();
    if(handle == 0)
      {
      lm.Pos += deltaPhys;
      }
    else if(handle == 1)
      {
      Vector3d headXSlice = m_Parent->MapImageToSlice(lm.Pos);
      Vector3d tailXSlice = m_Parent->MapPhysicalWindowToSlice(
            m_Parent->MapSliceToPhysicalWindow(headXSlice) + lm.Offset);
      Vector3d tailXImage = m_Parent->MapSliceToImage(tailXSlice);
      Vector3d tailXImageMoved = tailXImage + deltaPhys;
      Vector3d tailXSliceMoved = m_Parent->MapImageToSlice(tailXImageMoved);
      Vector2d tailXPhysWinMoved = m_Parent->MapSliceToPhysicalWindow(tailXSliceMoved);
      lm.Offset = tailXPhysWinMoved - m_Parent->MapSliceToPhysicalWindow(headXSlice);
      }

    lma->SetLandmark(lm);
    }


}

bool AnnotationModel::GetSelectedLandmarkTextValue(std::string &value)
{
  // A landmark annotation must be selected
  LandmarkAnnotation *asel = dynamic_cast<LandmarkAnnotation *>(this->GetSingleSelectedAnnotation());
  if(!asel)
    return false;

  // Get the value
  value = asel->GetLandmark().Text;
  return true;
}

void AnnotationModel::SetSelectedLandmarkTextValue(std::string value)
{
  LandmarkAnnotation *asel = dynamic_cast<LandmarkAnnotation *>(this->GetSingleSelectedAnnotation());
  assert(asel);

  annot::Landmark lmk = asel->GetLandmark();
  lmk.Text = value;
  asel->SetLandmark(lmk);
  this->InvokeEvent(ModelUpdateEvent());
}

bool AnnotationModel::GetSelectedAnnotationTagsValue(TagList &value)
{
  // A landmark annotation must be selected
  AbstractAnnotation *asel = this->GetSingleSelectedAnnotation();
  if(!asel)
    return false;

  value = asel->GetTags();
  return true;
}

void AnnotationModel::SetSelectedAnnotationTagsValue(TagList value)
{
  AbstractAnnotation *asel = this->GetSingleSelectedAnnotation();
  assert(asel);

  asel->SetTags(value);
  this->InvokeEvent(ModelUpdateEvent());
}

bool AnnotationModel::GetSelectedAnnotationColorValue(Vector3ui &value)
{
  // A landmark annotation must be selected
  AbstractAnnotation *asel = this->GetSingleSelectedAnnotation();
  if(!asel)
    return false;

  value = asel->GetColor3ui();
  return true;
}

void AnnotationModel::SetSelectedAnnotationColorValue(Vector3ui value)
{
  AbstractAnnotation *asel = this->GetSingleSelectedAnnotation();
  assert(asel);
  asel->SetColor3ui(value);
  this->InvokeEvent(ModelUpdateEvent());
}

annot::AbstractAnnotation *
AnnotationModel::GetSelectedHandleUnderCusror(const Vector3d &xSlice, int &out_handle)
{
  // Get the annotation data
  ImageAnnotationData *adata = this->GetAnnotations();

  out_handle = -1;
  for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    if(this->IsAnnotationVisible(*it) && (*it)->GetSelected())
      {
      // Draw all the line segments
      annot::LineSegmentAnnotation *lsa =
          dynamic_cast<annot::LineSegmentAnnotation *>(it->GetPointer());
      if(lsa)
        {
        // Draw the line
        Vector3d p1 = m_Parent->MapImageToSlice(lsa->GetSegment().first);
        Vector3d p2 = m_Parent->MapImageToSlice(lsa->GetSegment().second);

        // Test if either of these points is close to the cursor
        if(this->TestPointInClickRadius(xSlice, p1, 5))
          out_handle = 0;
        else if(this->TestPointInClickRadius(xSlice, p2, 5))
          out_handle = 1;
        }

      annot::LandmarkAnnotation *lma =
          dynamic_cast<annot::LandmarkAnnotation *>(it->GetPointer());
      if(lma)
        {
        Vector3d xHeadSlice, xTailSlice;
        annot::Landmark lm = lma->GetLandmark();
        this->GetLandmarkArrowPoints(lm, xHeadSlice, xTailSlice);

        // Test if either of these points is close to the cursor
        if(this->TestPointInClickRadius(xSlice, xHeadSlice, 5))
          out_handle = 0;
        else if(this->TestPointInClickRadius(xSlice, xTailSlice, 5))
          out_handle = 1;
        }

      if(out_handle >= 0)
        return it->GetPointer();
      }
    }

  return NULL;
}

bool AnnotationModel::CheckState(AnnotationModel::UIState state)
{
  switch(state)
    {
    case AnnotationModel::UIF_LINE_MODE:
      return this->GetAnnotationMode() == ANNOTATION_RULER;
    case AnnotationModel::UIF_LANDMARK_MODE:
      return this->GetAnnotationMode() == ANNOTATION_LANDMARK;
    case AnnotationModel::UIF_LINE_MODE_DRAWING:
      // return this->IsDrawingRuler();
      return this->GetFlagDrawingLine();
    case AnnotationModel::UIF_EDITING_MODE:
      return this->GetAnnotationMode() == ANNOTATION_SELECT;
    case AnnotationModel::UIF_SELECTION_SINGLE:
      return this->GetAnnotationCount(true,true) == 1;
    case AnnotationModel::UIF_SELECTION_ANY:
      return this->GetAnnotationCount(true,true) > 0;
    case AnnotationModel::UIF_ANNOTATIONS_EXIST:
      return this->GetAnnotationCount(false,false) > 0;
    }

  return false;
}

bool AnnotationModel::IsHoveringOverAnnotation(const Vector3d &xSlice)
{
  return (this->GetAnnotationUnderCursor(xSlice) != NULL);
}

void AnnotationModel::GetLandmarkArrowPoints(const annot::Landmark &lm,
                                             Vector3d &outHeadXSlice, Vector3d &outTailXSlice)
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

  // Create abstract models
  m_SelectedLandmarkTextModel = wrapGetterSetterPairAsProperty(
                                  this,
                                  &Self::GetSelectedLandmarkTextValue,
                                  &Self::SetSelectedLandmarkTextValue);

  m_SelectedAnnotationTagsModel = wrapGetterSetterPairAsProperty(
                                    this,
                                    &Self::GetSelectedAnnotationTagsValue,
                                    &Self::SetSelectedAnnotationTagsValue);

  m_SelectedAnnotationColorModel = wrapGetterSetterPairAsProperty(
                                     this,
                                     &Self::GetSelectedAnnotationColorValue,
                                     &Self::SetSelectedAnnotationColorValue);
}

AnnotationModel::~AnnotationModel()
{

}
