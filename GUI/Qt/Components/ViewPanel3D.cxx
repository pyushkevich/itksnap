#include "ViewPanel3D.h"
#include "ui_ViewPanel3D.h"
#include "GlobalUIModel.h"
#include "Generic3DModel.h"
#include "Generic3DRenderer.h"
#include "itkCommand.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageMeshLayers.h"
#include "StandaloneMeshWrapper.h"
#include <QMessageBox>
#include "MainImageWindow.h"
#include "SNAPQtCommon.h"
#include "QtWidgetActivator.h"
#include "DisplayLayoutModel.h"
#include <QtCore>
#include <qtconcurrentrun.h>
#include "itkProcessObject.h"
#include <QMenu>
#include "QtWidgetCoupling.h"
#include "QtActionCoupling.h"




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
  m_DropMenu->addAction(ui->actionImport_Viewpoint);
  m_DropMenu->addAction(ui->actionExport_Viewpoint);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionContinuous_Update);
  ui->actionContinuous_Update->setObjectName("actionContinuousUpdate");
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

  if(bucket.HasEvent(ActiveLayerChangeEvent()),
     m_Model->GetParentUI()->GetDriver()->GetIRISImageData()->GetMeshLayers())
    {
    ApplyDefaultColorBarVisibility();
    }
}

void ViewPanel3D::ApplyDefaultColorBarVisibility()
{
  if (!m_Model->GetParentUI()->GetDriver()->IsMainImageLoaded())
    return;

  // Do nothing if user has changed color bar visibility
  if (m_ColorBarUserInputOverride)
    return;

  ImageMeshLayers *mesh_layers =
      m_Model->GetParentUI()->GetDriver()->GetIRISImageData()->GetMeshLayers();

  auto active_layer = mesh_layers->GetLayer(mesh_layers->GetActiveLayerId());

  // we only turn on color bar by default for standalone mesh layers
  if (dynamic_cast<StandaloneMeshWrapper*>(active_layer.GetPointer()))
    {
    if (!ui->btnColorBar->isChecked())
      {
      ui->btnColorBar->setChecked(true);
      on_btnColorBar_clicked(false); // click the button programmatically
      }
    }
  else
    {
    if (ui->btnColorBar->isChecked())
      {
      ui->btnColorBar->setChecked(false);
      on_btnColorBar_clicked(false); // click the button programmatically
      }
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
  activateOnFlag(ui->btnFlip, m_Model, Generic3DModel::UIF_FLIP_ENABLED);
  activateOnFlag(ui->btnUpdateMesh, m_Model, Generic3DModel::UIF_MESH_DIRTY);
  activateOnFlag(ui->actionRestore_Viewpoint, m_Model, Generic3DModel::UIF_CAMERA_STATE_SAVED);

  // Listen to layout events
  connectITK(m_Model->GetParentUI()->GetDisplayLayoutModel(),
             DisplayLayoutModel::ViewPanelLayoutChangeEvent());

  // Listen to changes in active tool to adjust button visibility
  connectITK(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3DModel(),
             ValueChangedEvent());

  // Listen to changes in layer change for 3d view
  connectITK(globalUI->GetDriver()->GetIRISImageData()->GetMeshLayers(),
             ActiveLayerChangeEvent());


  makeCoupling(ui->actionContinuous_Update, m_Model->GetContinuousUpdateModel());

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

void ViewPanel3D::on_btnColorBar_clicked(bool isUser)
{
  if (isUser)
    m_ColorBarUserInputOverride = true;

  m_Model->SetDisplayColorBar(!m_Model->GetDisplayColorBar());
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
      m_RenderFuture = QtConcurrent::run(&ViewPanel3D::UpdateMeshesInBackground, this);
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

void ViewPanel3D::LoadCameraViewpoint(QString file)
{
  QFile loadFile(file);
  if (!loadFile.open(QIODevice::ReadOnly))
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("I/O error: ") + loadFile.errorString());

  QByteArray loadData = loadFile.readAll();
  QJsonParseError jsonError;
  QJsonDocument loadDoc(QJsonDocument::fromJson(loadData, &jsonError));

  if (loadDoc.isNull())
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: ") + jsonError.errorString());

  QJsonObject json = loadDoc.object();

  // QJsonArray to Vector3d
  if (!json["position"].isArray()
      || json["position"].toArray().count() != 3)
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: position: expected an array of size 3"));
    return;
  }
  Vector3d camPosition(
      json["position"][0].toDouble(),
      json["position"][1].toDouble(),
      json["position"][2].toDouble());

  // QJsonArray to Vector3d
  if (!json["focal_point"].isArray()
      || json["focal_point"].toArray().count() != 3)
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: focal_point: expected an array of size 3"));
    return;
  }
  Vector3d camFocalPoint(
      json["focal_point"][0].toDouble(),
      json["focal_point"][1].toDouble(),
      json["focal_point"][2].toDouble());

  // QJsonArray to Vector3d
  if (!json["view_up"].isArray()
      || json["view_up"].toArray().count() != 3)
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: view_up: expected an array of size 3"));
    return;
  }
  Vector3d camViewUp(
      json["view_up"][0].toDouble(),
      json["view_up"][1].toDouble(),
      json["view_up"][2].toDouble());

  // QJsonArray to Vector2d
  if (!json["clipping_range"].isArray()
      || json["clipping_range"].toArray().count() != 2)
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: clipping_range: expected an array of size 2"));
    return;
  }
  Vector2d camClippingRange(
      json["clipping_range"][0].toDouble(),
      json["clipping_range"][1].toDouble());

  // QJsonValue to double
  if (!json["view_angle"].isDouble())
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: view_angle: expected a number"));
    return;
  }
  double camViewAngle = json["view_angle"].toDouble();

  // QJsonValue to double
  if (!json["parallel_scale"].isDouble())
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: parallel_scale: expected a number"));
    return;
  }
  double camParallelScale = json["parallel_scale"].toDouble();

  // QJsonValue to int (boolean)
  if (!json["parallel_projection"].isBool())
  {
    QMessageBox::warning(this, "Import camera viewpoint error",
        QString("JSON error: parallel_projection: expected a boolean"));
    return;
  }
  int camParallelProjection = json["parallel_projection"].toBool();

  CameraState cam = {
    .position = camPosition,
    .focal_point = camFocalPoint,
    .view_up = camViewUp,
    .clipping_range = camClippingRange,
    .view_angle = camViewAngle,
    .parallel_scale = camParallelScale,
    .parallel_projection = camParallelProjection
  };
  m_Model->GetRenderer()->SetCameraState(cam);
}

void ViewPanel3D::SaveCameraViewpoint(QString file)
{
  CameraState cam = m_Model->GetRenderer()->GetCameraState();
  QJsonObject json;

  // Vector3d to QJsonArray
  QJsonArray camPosition;
  for (const double& x : cam.position)
    camPosition += x;

  // Vector3d to QJsonArray
  QJsonArray camFocalPoint;
  for (const double& x : cam.focal_point)
    camFocalPoint += x;

  // Vector3d to QJsonArray
  QJsonArray camViewUp;
  for (const double& x : cam.view_up)
    camViewUp += x;

  // Vector2d to QJsonArray
  QJsonArray camClippingRange;
  for (const double& x : cam.clipping_range)
    camClippingRange += x;

  // Fields from vtkCamera
  json["position"] = camPosition;
  json["focal_point"] = camFocalPoint;
  json["view_up"] = camViewUp;
  json["clipping_range"] = camClippingRange;
  json["view_angle"] = QJsonValue(cam.view_angle);
  json["parallel_scale"] = QJsonValue(cam.parallel_scale);
  json["parallel_projection"] = QJsonValue(cam.parallel_projection).toBool();

  QFile saveFile(file);
  if (!saveFile.open(QIODevice::WriteOnly)
      || saveFile.write(QJsonDocument(json).toJson()) == -1)
    QMessageBox::warning(this, "Export camera viewpoint error",
        QString("I/O error: ") + saveFile.errorString());
}

void ViewPanel3D::on_actionImport_Viewpoint_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleOpenDialogWithHistory(
        this, m_Model->GetParentUI(), "CameraViewpoint",
        "Load Camera Viewpoint - ITK-SNAP",
        "Camera Viewpoint JSON File",
        "Camera Files (*.json)");

  // Open the labels from the selection
  if(selection.length())
    LoadCameraViewpoint(selection);
}

void ViewPanel3D::on_actionExport_Viewpoint_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleSaveDialogWithHistory(
        this, m_Model->GetParentUI(), "CameraViewpoint",
        "Save Camera Viewpoint - ITK-SNAP",
        "Camera Viewpoint JSON File",
        "Camera Files (*.json)",
        true);

  // Open the labels from the selection
  if(selection.length())
    SaveCameraViewpoint(selection);
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
