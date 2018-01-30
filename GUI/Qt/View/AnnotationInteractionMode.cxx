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

  m_ParentPanel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
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

#include <QInputDialog>
#include "SNAPQtCommon.h"

void AnnotationInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessPushEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier))
      ev->accept();

    if(m_Model->IsMovingSelection())
      m_ParentPanel->setCursor(Qt::ClosedHandCursor);
    }
}

void AnnotationInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();

  // Nothing is pressed
  if(this->isHovering())
    {
    if(m_Model->ProcessMoveEvent(m_XSlice, false, false))
      ev->accept();

    if(m_Model->GetAnnotationMode() == ANNOTATION_SELECT && m_Model->IsHoveringOverAnnotation(m_XSlice))
      {
      m_ParentPanel->setCursor(Qt::OpenHandCursor);
      }
    else
      {
      m_ParentPanel->setCursor(Qt::ArrowCursor);
      }
    }

  // Left press was accepted
  else if(this->isDragging())
    {
    if(m_Model->ProcessMoveEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier, true))
      ev->accept();
    }
}

#include <QTimer>

void AnnotationInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  ev->ignore();

  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessReleaseEvent(m_XSlice, ev->modifiers() == Qt::ShiftModifier))
      {
      ev->accept();

      // If the user is done drawing the text annotation arrow
      if(m_Model->GetAnnotationMode() == ANNOTATION_LANDMARK && m_Model->GetFlagDrawingLine() == false)
        QTimer::singleShot(1, this, SLOT(onTextInputRequested()));

      else if(m_Model->GetAnnotationMode() == ANNOTATION_RULER && m_Model->GetFlagDrawingLine() == false)
        m_Model->AcceptLine();
      }

    // Handle cursor changes
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
}

void AnnotationInteractionMode::onAcceptAction()
{
  m_Model->AcceptLine();
}

void AnnotationInteractionMode::onModelUpdate(const EventBucket &bucket)
{
  this->update();
}

void AnnotationInteractionMode::onTextInputRequested()
{
  // Special handling in text annotation mode
  bool ok;
  QString text = QInputDialog::getText(this, "Text Annotation", "Enter annotation text:", QLineEdit::Normal,
                                       QString(), &ok);
  if(ok && text.length())
    {
    m_Model->SetCurrentAnnotationText(to_utf8(text));
    m_Model->AcceptLine();
    }
}



