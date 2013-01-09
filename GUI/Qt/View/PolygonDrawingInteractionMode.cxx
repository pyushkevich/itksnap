#include "PolygonDrawingInteractionMode.h"
#include "PolygonDrawingRenderer.h"
#include "PolygonDrawingModel.h"
#include "QtAbstractOpenGLBox.h"
#include "QtWarningDialog.h"
#include "QtWidgetActivator.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "GenericSliceView.h"

#include <QMenu>

QAction *setupAction(PolygonDrawingInteractionMode *w, QMenu *menu,
                     QString icon, QString sshort, QString slong,
                     const char *slotname,
                     PolygonDrawingUIState flag1,
                     PolygonDrawingUIState flag2)
{
  QAction *action = new QAction(w);
  action->setText(slong);
  action->setIconText(sshort);
  if(icon.length())
    action->setIcon(QIcon(QString(":/root/%1.png").arg(icon)));

  // activateOnAllFlags(action, w->GetModel(), flag1, flag2);

  menu->addAction(action);

  // connect(action, SIGNAL(triggered()), w, slotname);

  return action;
}


PolygonDrawingInteractionMode
::PolygonDrawingInteractionMode(GenericSliceView *parent) :
    SliceWindowInteractionDelegateWidget(parent)
{
  m_Renderer = PolygonDrawingRenderer::New();
  m_Renderer->SetParentRenderer(
        static_cast<GenericSliceRenderer *>(parent->GetRenderer()));
  m_Model = NULL;
}

PolygonDrawingInteractionMode
::~PolygonDrawingInteractionMode()
{

}

void
PolygonDrawingInteractionMode
::SetModel(PolygonDrawingModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);
  SetParentModel(model->GetParent());

  // Listen to events in the model (update buttons)
  connectITK(m_Model, StateMachineChangeEvent());
}

void PolygonDrawingInteractionMode::onModelUpdate(const EventBucket &bucket)
{
  update();
}

void PolygonDrawingInteractionMode::mousePressEvent(QMouseEvent *ev)
{
  // Call the model's code
  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessPushEvent(m_XSlice(0), m_XSlice(1),
                                 ev->modifiers().testFlag(Qt::ShiftModifier)))
      {
      ev->accept();
      }
    }
}

void PolygonDrawingInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(m_LeftDown)
    {
    if(m_Model->ProcessDragEvent(m_XSlice(0), m_XSlice(1)))
      {
      ev->accept();
      }
    }
  else if (!isDragging())
    {
    if(m_Model->ProcessMouseMoveEvent(m_XSlice(0), m_XSlice(1)))
      {
      ev->accept();
      m_ParentView->update();
      }
    }
}

void PolygonDrawingInteractionMode::mouseReleaseEvent(QMouseEvent *ev)
{
  if(ev->button() == Qt::LeftButton)
    {
    if(m_Model->ProcessReleaseEvent(m_XSlice(0), m_XSlice(1)))
      {
      ev->accept();
      }
    }
}

void PolygonDrawingInteractionMode
::contextMenuEvent(QContextMenuEvent *ev)
{
  // We bring up the context menu on the right click only if the option
  // is enabled. But we always bring it up if there are modifiers
  if(m_ParentModel->GetDriver()->GetGlobalState()->GetPolygonDrawingContextMenu()
     || ev->modifiers().testFlag(Qt::ControlModifier)
     || ev->modifiers().testFlag(Qt::MetaModifier))
    {
    emit contextMenuRequested();
    }
}

/* ==============================
   SLOTS
   ============================== */

void PolygonDrawingInteractionMode::onPastePolygon()
{
  m_Model->PastePolygon();
}


void PolygonDrawingInteractionMode::onAcceptPolygon()
{
  // Make current GL context current before running any GL operations
  QtAbstractOpenGLBox *parent = this->GetParentGLWidget();
  parent->makeCurrent();

  // This is hacky, but we need the viewport in the parent GL widget to be
  // set to the entire window. This is needed because the rendering of the
  // slice view may set up multiple viewports, and for polygon rasterization
  // we only need one.
  glViewport(0, 0, parent->width(), parent->height());

  // Create a warning list
  std::vector<IRISWarning> warnings;

  // Call accept polygon code
  m_Model->AcceptPolygon(warnings);

  // Display the warnings
  if(warnings.size())
    {
    QtWarningDialog::show(warnings);
    }
}


void PolygonDrawingInteractionMode::onSplitSelected()
{
  m_Model->Insert();
}


void PolygonDrawingInteractionMode::onDeleteSelected()
{
  m_Model->Delete();
}


void PolygonDrawingInteractionMode::onClearPolygon()
{
  m_Model->Reset();
}


void PolygonDrawingInteractionMode::onCloseLoopAndEdit()
{
  m_Model->ClosePolygon();
}


void PolygonDrawingInteractionMode::onCloseLoopAndAccept()
{
  onCloseLoopAndEdit();
  onAcceptPolygon();
}


void PolygonDrawingInteractionMode::onUndoLastPoint()
{
  m_Model->DropLastPoint();
}


void PolygonDrawingInteractionMode::onCancelDrawing()
{
  m_Model->Reset();
}

#include <SliceViewPanel.h>

void PolygonDrawingInteractionMode::enterEvent(QEvent *)
{
  // TODO: this is hideous!
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(true);
}

void PolygonDrawingInteractionMode::leaveEvent(QEvent *)
{
  SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
  panel->SetMouseMotionTracking(false);
}




