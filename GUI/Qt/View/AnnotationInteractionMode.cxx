#include "AnnotationInteractionMode.h"
#include "AnnotationRenderer.h"
#include "AnnotationModel.h"
#include "GenericSliceView.h"
#include "SliceViewPanel.h"

AnnotationInteractionMode::AnnotationInteractionMode(GenericSliceView *parent)
  : SliceWindowInteractionDelegateWidget(parent)
{
  m_Renderer = AnnotationRenderer::New();
  m_Renderer->SetParentRenderer(
        static_cast<GenericSliceRenderer *>(parent->GetRenderer()));
  m_Model = NULL;
}

AnnotationInteractionMode::~AnnotationInteractionMode()
{

}

void AnnotationInteractionMode::SetModel(AnnotationModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  SetParentModel(model->GetParent());

  connectITK(m_Model, StateMachineChangeEvent());
  connectITK(m_Model, ModelUpdateEvent());
}

void AnnotationInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());

  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessPushEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier))
      ev->accept();

    if(m_Model->IsMovingSelection())
      panel->setCursor(Qt::ClosedHandCursor);
    }
  else if(ev->button() == Qt::RightButton)
    {
    if(m_Model->IsDrawingRuler())
      m_Model->AcceptLine();
    }
}

void AnnotationInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  if(m_LeftDown)
    {
    if(m_Model->ProcessDragEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier))
      ev->accept();
    }
  else if(m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
    {
    SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
    if(m_Model->IsHoveringOverAnnotation(m_XSlice))
      {
      panel->setCursor(Qt::OpenHandCursor);
      }
    else
      {
      panel->setCursor(Qt::ArrowCursor);
      }

    }
}

void AnnotationInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());

  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessReleaseEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier))
      ev->accept();
    }

  if(m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
    {
    if(m_Model->IsHoveringOverAnnotation(m_XSlice))
      {
      panel->setCursor(Qt::OpenHandCursor);
      }
    else
      {
      panel->setCursor(Qt::ArrowCursor);
      }
    }
}

void AnnotationInteractionMode::onAcceptAction()
{
  m_Model->AcceptLine();
}

void AnnotationInteractionMode::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}



