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
#include <QtCore>
#include <qtconcurrentrun.h>
#include "itkProcessObject.h"
#include <QMenu>




ViewPanel3D::ViewPanel3D(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ViewPanel3D)
{
  ui->setupUi(this);

  // Set up timer for continuous update rendering
  m_RenderTimer = new QTimer();
  m_RenderTimer->setInterval(100);
  connect(m_RenderTimer, SIGNAL(timeout()), SLOT(onTimer()));

  // Create a progress command
  m_RenderProgressCommand = CommandType::New();
  m_RenderProgressCommand->SetCallbackFunction(this, &ViewPanel3D::ProgressCallback);

  // Connect the progress event
  ui->progressBar->setRange(0, 1000);
  QObject::connect(this, SIGNAL(renderProgress(int)), ui->progressBar, SLOT(setValue(int)),
                   Qt::DirectConnection);

  // Set up the context menu
  m_DropMenu = new QMenu(this);
  m_DropMenu->setStyleSheet("font-size:11px;");
  m_DropMenu->addAction(ui->actionReset_Viewpoint);
  m_DropMenu->addAction(ui->actionSave_Viewpoint);
  m_DropMenu->addAction(ui->actionRestore_Viewpoint);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionContinuous_Update);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionClear_Rendering);

  // Make the actions globally accessible
  this->addActions(m_DropMenu->actions());
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
  GlobalState *gs = m_Model->GetParentUI()->GetGlobalState();
  if(bucket.HasEvent(DisplayLayoutModel::ViewPanelLayoutChangeEvent()))
    {
    UpdateExpandViewButton();
    }

  if(bucket.HasEvent(ValueChangedEvent(), gs->GetToolbarMode3DModel()))
    {
    UpdateActionButtons();
    }
}

void ViewPanel3D::on_btnUpdateMesh_clicked()
{
  try
    {
    // Tell the model to update itself
    m_Model->UpdateSegmentationMesh(m_Model->GetParentUI()->GetProgressCommand());
    // m_Model->UpdateSegmentationMesh(m_RenderProgressCommand);
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
  activateOnFlag(ui->btnFlip, m_Model, Generic3DModel::UIF_FLIP_ENABLED);
  activateOnFlag(ui->btnUpdateMesh, m_Model, Generic3DModel::UIF_MESH_DIRTY);
  activateOnFlag(ui->actionRestore_Viewpoint, m_Model, Generic3DModel::UIF_CAMERA_STATE_SAVED);

  // Listen to layout events
  connectITK(m_Model->GetParentUI()->GetDisplayLayoutModel(),
             DisplayLayoutModel::ViewPanelLayoutChangeEvent());

  // Listen to changes in active tool to adjust button visibility
  connectITK(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3DModel(),
             ValueChangedEvent());

  // Set up the buttons
  this->UpdateActionButtons();

  // Start the timer
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

// This method is run in a concurrent thread
void ViewPanel3D::UpdateMeshesInBackground()
{
  // Make sure the model actually requires updating
  if(m_Model && m_Model->CheckState(Generic3DModel::UIF_MESH_DIRTY))
    {
    m_Model->UpdateSegmentationMesh(m_RenderProgressCommand);
    }
}

void ViewPanel3D::ProgressCallback(itk::Object *source, const itk::EventObject &)
{
  itk::ProcessObject *po = static_cast<itk::ProcessObject *>(source);

  m_RenderProgressMutex.lock();
  m_RenderProgressValue = po->GetProgress();
  m_RenderProgressMutex.unlock();
}

void ViewPanel3D::on_btnScreenshot_clicked()
{
  MainImageWindow *window = findParentWidget<MainImageWindow>(this);
  window->ExportScreenshot(3);
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

void ViewPanel3D::onTimer()
{
  if(!m_RenderFuture.isRunning())
    {
    // Does work need to be done?
    if(m_Model && ui->actionContinuous_Update->isChecked()
       && m_Model->CheckState(Generic3DModel::UIF_MESH_DIRTY))
      {
      // Launch the worker thread
      m_RenderProgressValue = 0;
      m_RenderElapsedTicks = 0;
      m_RenderFuture = QtConcurrent::run(this, &ViewPanel3D::UpdateMeshesInBackground);
      }
    else
      {
      ui->progressBar->setVisible(false);
      }
    }
  else
    {
    // We only want to show progress after some minimum timeout (1 sec)
    if((++m_RenderElapsedTicks) > 10)
      {
      ui->progressBar->setVisible(true);

      m_RenderProgressMutex.lock();
      emit renderProgress((int)(1000 * m_RenderProgressValue));
      m_RenderProgressMutex.unlock();
      }
    }
}

void ViewPanel3D::on_actionReset_Viewpoint_triggered()
{
  m_Model->ResetView();
}

void ViewPanel3D::on_actionSave_Viewpoint_triggered()
{
  m_Model->SaveCameraState();
}

void ViewPanel3D::on_actionRestore_Viewpoint_triggered()
{
  m_Model->RestoreCameraState();
}

void ViewPanel3D::on_actionContinuous_Update_triggered()
{
  ui->btnUpdateMesh->setVisible(!ui->actionContinuous_Update->isChecked());
}

void ViewPanel3D::on_btnMenu_pressed()
{
  m_DropMenu->popup(QCursor::pos());
  ui->btnMenu->setDown(false);
}

void ViewPanel3D::on_btnFlip_clicked()
{
  m_Model->FlipAction();
}

void ViewPanel3D::UpdateActionButtons()
{
  ToolbarMode3DType mode = m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3D();
  switch(mode)
    {
    case TRACKBALL_MODE:
    case CROSSHAIRS_3D_MODE:
      ui->btnAccept->setVisible(false);
      ui->btnCancel->setVisible(false);
      ui->btnFlip->setVisible(false);
      break;
    case SPRAYPAINT_MODE:
      ui->btnAccept->setVisible(true);
      ui->btnCancel->setVisible(true);
      ui->btnFlip->setVisible(false);
      break;
    case SCALPEL_MODE:
      ui->btnAccept->setVisible(true);
      ui->btnCancel->setVisible(true);
      ui->btnFlip->setVisible(true);
      break;
    }
}

void ViewPanel3D::on_actionClear_Rendering_triggered()
{
  m_Model->ClearRenderingAction();

}
