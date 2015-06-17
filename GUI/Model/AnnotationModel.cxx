#include "AnnotationModel.h"
#include "GenericSliceModel.h"

void AnnotationModel::SetMode(AnnotationModel::Mode mode)
{
  m_Mode = mode;
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

double AnnotationModel::GetAngleBetweenLines(const AnnotationModel::LineIntervalType &l1, const AnnotationModel::LineIntervalType &l2)
{
  Vector2f v1 = m_Parent->MapSliceToPhysicalWindow(l1.first)  - m_Parent->MapSliceToPhysicalWindow(l1.second);
  v1 /= sqrt(v1[0]*v1[0] + v1[1]*v1[1]);

  Vector2f v2 = m_Parent->MapSliceToPhysicalWindow(l2.first)
                 - m_Parent->MapSliceToPhysicalWindow(l2.second);
  v2 /= sqrt(v2[0]*v2[0] + v2[1]*v2[1]);

  // Compute the dot product and no need for the third components that are zeros
  double angle = 180.0 * acos(fabs(v1[0]*v2[0]+v1[1]*v2[1])) / vnl_math::pi;
  return angle;
}

bool AnnotationModel::ProcessPushEvent(const Vector3d &xSlice)
{
  bool handled = false;
  if(m_Mode == LINE_DRAWING)
    {
    if(m_FlagDrawingLine)
      {
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

  if(handled)
    InvokeEvent(ModelUpdateEvent());
  return handled;


  /*
   *
   *     // Record the location
    Vector3f xEvent = m_Parent->MapWindowToSlice(event.XSpace.extract(2));
    // Handle different mouse buttons
    if(Fl::event_button() == FL_LEFT_MOUSE)
      // Move the second point around the first point which is fixed
      m_CurrentLine.second = xEvent;
    else if (Fl::event_button() == FL_MIDDLE_MOUSE)
      {
      // Translation of the segment
      Vector3f delta = xEvent - m_CurrentLine.second;
   m_CurrentLine.second = xEvent;
      m_CurrentLine.first += delta;
      }
    else if(Fl::event_button() == FL_RIGHT_MOUSE)
      {
      // Commit the segment
      m_Lines.push_back(m_CurrentLine);
      m_FlagDrawingLine = false;
      }
    // Redraw
    m_Parent->GetCanvas()->redraw();

    // Even though no action may have been performed, we don't want other handlers
    // to get the left and right mouse button events
    return 1;
    }
  else
    return 0;*/
}

bool AnnotationModel::ProcessDragEvent(const Vector3d &xSlice)
{
  bool handled = false;
  if(m_Mode == LINE_DRAWING)
    {
    if(m_FlagDrawingLine)
      {
      m_CurrentLine.second = to_float(xSlice);
      handled = true;
      }
    }

  if(handled)
    this->InvokeEvent(ModelUpdateEvent());
  return handled;
}

bool AnnotationModel::ProcessReleaseEvent(const Vector3d &xSlice)
{
  return this->ProcessDragEvent(xSlice);
}

void AnnotationModel::AcceptLine()
{
  m_Lines.push_back(m_CurrentLine);
  m_FlagDrawingLine = false;
  this->InvokeEvent(ModelUpdateEvent());
}

bool AnnotationModel::CheckState(AnnotationModel::UIState state)
{
  switch(state)
    {
    case AnnotationModel::UIF_LINE_MODE:
      return m_Mode == LINE_DRAWING;
      break;
    case AnnotationModel::UIF_LINE_MODE_DRAWING:
      return m_Mode == LINE_DRAWING && m_FlagDrawingLine;
      break;
    case AnnotationModel::UIF_EDITING_MODE:
      return m_Mode == SELECT;
      break;

    }

  return false;
}

AnnotationModel::AnnotationModel()
{
  m_FlagDrawingLine = false;
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

AnnotationModel::~AnnotationModel()
{

}

