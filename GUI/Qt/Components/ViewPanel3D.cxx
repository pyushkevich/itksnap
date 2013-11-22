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





ViewPanel3D::ViewPanel3D(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ViewPanel3D)
{
  ui->setupUi(this);
  m_RenderWorker = new RenderWorker();

  m_RenderTimer = new QTimer();
  m_RenderTimer->setInterval(100);
  connect(m_RenderTimer, SIGNAL(timeout()), SLOT(onTimer()));
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

  m_RenderWorker->SetModel(m_Model);
  m_RenderTimer->start();
}

void ViewPanel3D::UpdateExpandViewButton()
{
  // Get the layout a pplied when the button is pressed
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

void ViewPanel3D::onWorkerUpdate()
{
  this->update();
}

#include <QtCore>
#include <qtconcurrentrun.h>
void ViewPanel3D::onTimer()
{
  static QFuture<void> future;
  if(!future.isRunning())
    future = QtConcurrent::run(m_RenderWorker, &RenderWorker::timerHit);
}
