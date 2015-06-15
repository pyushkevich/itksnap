#include "AnnotationInteractionMode.h"
#include "AnnotationRenderer.h"
#include "AnnotationModel.h"
#include "GenericSliceView.h"

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
  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessPushEvent(m_XSlice))
      ev->accept();
    }
  else if(ev->button() == Qt::RightButton)
    {
    if(m_Model->GetMode() == AnnotationModel::LINE_DRAWING && m_Model->GetFlagDrawingLine())
      m_Model->AcceptLine();
    }
}

void AnnotationInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  if(m_LeftDown)
    {
    if(m_Model->ProcessDragEvent(m_XSlice))
      ev->accept();
    }
}

void AnnotationInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessReleaseEvent(m_XSlice))
      ev->accept();
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



