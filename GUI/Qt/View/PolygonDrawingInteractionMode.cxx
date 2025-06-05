#include "PolygonDrawingInteractionMode.h"
#include "PolygonDrawingModel.h"
#include "QtWarningDialog.h"
#include "QtWidgetActivator.h"
#include "GenericSliceModel.h"
#include "IRISApplication.h"
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


PolygonDrawingInteractionMode ::PolygonDrawingInteractionMode(QWidget *parent, QWidget *canvasWidget)
  : SliceWindowInteractionDelegateWidget(parent, canvasWidget)
{
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

#include <vtkRenderWindow.h>

void PolygonDrawingInteractionMode::mouseMoveEvent(QMouseEvent *ev)
{
  ev->ignore();
  if(this->m_LeftStatus == PRESS_ACCEPTED)
    {
    if(m_Model->ProcessDragEvent(m_XSlice(0), m_XSlice(1)))
      ev->accept();
    }
  else if (this->isHovering())
    {
    if(m_Model->ProcessMouseMoveEvent(m_XSlice(0), m_XSlice(1)))
      {
      ev->accept();
      this->updateClient();
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
  m_Model->SplitSelectedEdges();
}


void PolygonDrawingInteractionMode::onDeleteSelected()
{
  m_Model->DeleteSelected();
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

void
PolygonDrawingInteractionMode::enterEvent(QEnterEvent *)
{
  this->setMouseMotionTracking(true);
}

void PolygonDrawingInteractionMode::leaveEvent(QEvent *)
{
  this->setMouseMotionTracking(false);
}




