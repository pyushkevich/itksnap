#include "ViewPanel3D.h"
#include "ui_ViewPanel3D.h"
#include "GlobalUIModel.h"
#include "Generic3DModel.h"
#include "itkCommand.h"
#include "IRISException.h"
#include <QMessageBox>
#include "MainImageWindow.h"
#include "SNAPQtCommon.h"
#include "QtWidgetActivator.h"
#include "DisplayLayoutModel.h"

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

// Just playing around, this does not work
#ifdef RENDERTHREAD
class RenderThread : public QThread
{
  Q_OBJECT

public:
  RenderThread(QObject *parent = 0);
  ~RenderThread();

  /** We call this when the segmentation image has changed or when the
   * user presses the update button */
  void UpdateScene();

signals:

  void updatedScene();

protected:

  void run();

private:

  QMutex mutex;
  QWaitCondition condition;
  bool restart;
  bool abort;


};


RenderThread::RenderThread(QObject *parent)
  : QThread(parent)
{
  restart = false;
  abort = false;
}

RenderThread::~RenderThread()
{
  mutex.lock();
  abort = true;
  condition.wakeOne();
  mutex.unlock();

  wait();
}

void RenderThread::UpdateScene()
{
  QMutexLocker locker(&mutex);
  if (!isRunning())
    {
    start(LowPriority);
    }
  else
    {
    restart = true;
    condition.wakeOne();
    }
}

void RenderThread::run()
{
  // What do we do?
  // m_Model
}

#endif


ViewPanel3D::ViewPanel3D(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ViewPanel3D)
{
  ui->setupUi(this);
}

ViewPanel3D::~ViewPanel3D()
{
  delete ui;
}

GenericView3D *ViewPanel3D::Get3DView()
{
  return ui->view3d;
}

void ViewPanel3D::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DisplayLayoutModel::ViewPanelLayoutChangeEvent()))
    {
    UpdateExpandViewButton();
    }
}

void ViewPanel3D::on_btnUpdateMesh_clicked()
{
  try
    {
    // Tell the model to update itself
    m_Model->UpdateSegmentationMesh(m_Model->GetParentUI()->GetProgressCommand());
    }
  catch(IRISException & IRISexc)
    {
    QMessageBox::warning(this, "Problem generating mesh", IRISexc.what());
    }

  // TODO: Delete this later - should be automatic!
  ui->view3d->repaint();
}

void ViewPanel3D::Initialize(GlobalUIModel *globalUI)
{
  // Save the model
  m_GlobalUI = globalUI;
  m_Model = globalUI->GetModel3D();
  ui->view3d->SetModel(m_Model);

  // Set activations
  activateOnFlag(ui->btnAccept, m_Model, Generic3DModel::UIF_MESH_ACTION_PENDING);
  activateOnFlag(ui->btnCancel, m_Model, Generic3DModel::UIF_MESH_ACTION_PENDING);
  activateOnFlag(ui->btnUpdateMesh, m_Model, Generic3DModel::UIF_MESH_DIRTY);

  // Listen to layout events
  connectITK(m_Model->GetParentUI()->GetDisplayLayoutModel(),
             DisplayLayoutModel::ViewPanelLayoutChangeEvent());
}

void ViewPanel3D::UpdateExpandViewButton()
{
  // Get the layout applied when the button is pressed
  DisplayLayoutModel *dlm = m_GlobalUI->GetDisplayLayoutModel();
  DisplayLayoutModel::ViewPanelLayout layout =
      dlm->GetViewPanelExpandButtonActionModel(3)->GetValue();

  // Set the tooltip
  if(layout == DisplayLayoutModel::VIEW_ALL)
    {
    ui->btnExpand->setIcon(QIcon(":/root/dl_fourviews.png"));
    ui->btnExpand->setToolTip("Restore the four-panel display configuration");
    }
  else
    {
    ui->btnExpand->setIcon(QIcon(":/root/dl_3d.png"));
    ui->btnExpand->setToolTip("Expand the 3D view to occupy the entire window");
    }
}

void ViewPanel3D::on_btnScreenshot_clicked()
{
  MainImageWindow *window = findParentWidget<MainImageWindow>(this);
  window->ExportScreenshot(3);
}

void ViewPanel3D::on_btnResetView_clicked()
{
  m_Model->ResetView();
}

void ViewPanel3D::on_btnAccept_clicked()
{
  if(!m_Model->AcceptAction())
    {
    QMessageBox::information(this, "No voxels were updated",
                             "The 3D operation did not update any voxels in "
                             "the segmentation. Check that the foreground and "
                             "background labels are selected correctly.");
    }
}

void ViewPanel3D::on_btnCancel_clicked()
{
  m_Model->CancelAction();
}

void ViewPanel3D::on_btnExpand_clicked()
{
  // Get the layout applied when the button is pressed
  DisplayLayoutModel *dlm = m_GlobalUI->GetDisplayLayoutModel();
  DisplayLayoutModel::ViewPanelLayout layout =
      dlm->GetViewPanelExpandButtonActionModel(3)->GetValue();

  // Apply this layout
  dlm->GetViewPanelLayoutModel()->SetValue(layout);
}
