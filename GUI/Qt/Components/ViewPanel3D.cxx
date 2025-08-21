#include "ViewPanel3D.h"
#include "HistoryManager.h"
#include "LayerInspectorDialog.h"
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
#include <QtConcurrent/QtConcurrent>
#include "itkProcessObject.h"
#include <QMenu>
#include "QtWidgetCoupling.h"
#include "QtActionCoupling.h"
#include "QtMenuCoupling.h"
#include "MeshManager.h"

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
  QObject::connect(
    this, SIGNAL(renderProgress(int)), ui->progressBar, SLOT(setValue(int)), Qt::DirectConnection);

  QObject::connect(
    this, SIGNAL(updateContextMenu()), this, SLOT(onContextMenuUpdateRequested()), Qt::QueuedConnection);

  // Color bar toolbutton has an action
  ui->btnColorBar->setDefaultAction(ui->actionColorBar_Visible);

  // Set up the context menu in the top right corner
  m_ContextToolButton = CreateContextToolButton(ui->view3d);
  m_ContextButtonMenu = new QMenu(m_ContextToolButton);
  m_ContextButtonMenu->setStyleSheet("font-size:11px;");
  m_ContextToolButton->setMenu(m_ContextButtonMenu);

  // Listen for leave/enter events to active context button
  connect(ui->view3d, SIGNAL(mouseEntered()), m_ContextToolButton, SLOT(show()));
  connect(ui->view3d, SIGNAL(mouseLeft()), m_ContextToolButton, SLOT(hide()));
  connect(ui->view3d, SIGNAL(resized()), this, SLOT(onView3dResize()));

  // Create the focal point target submenu
  m_FocalPointTargetMenu = new QMenu(tr("Focus Camera on "), this);
  m_FocalPointTargetMenu->setStyleSheet("font-size:11px;");

  // Set up the camera context menu
  m_DropMenu = new QMenu(this);
  m_DropMenu->setStyleSheet("font-size:11px;");
  m_DropMenu->addAction(ui->actionReset_Viewpoint);
  m_DropMenu->addMenu(m_FocalPointTargetMenu);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionSave_Viewpoint);
  m_DropMenu->addAction(ui->actionRestore_Viewpoint);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionExport_Viewpoint);
  m_DropMenu->addAction(ui->actionImport_Viewpoint);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionContinuous_Update);
  ui->actionContinuous_Update->setObjectName("actionContinuousUpdate");
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionClear_Rendering);

  ui->btnMenu->setMenu(m_DropMenu);
  ui->btnMenu->setPopupMode(QToolButton::InstantPopup);

  // Menu for listing layers
  m_MeshLayerMenu = new QMenu(tr("Active Mesh"), this);
  m_MeshLayerMenu->setStyleSheet("font-size:11px;");
  m_DropMenu->addMenu(m_MeshLayerMenu);
  m_DropMenu->setVisible(false);

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

void
ViewPanel3D::onModelUpdate(const EventBucket &bucket)
{
  GlobalState *gs = m_Model->GetParentUI()->GetGlobalState();
  if (bucket.HasEvent(DisplayLayoutModel::ViewPanelLayoutChangeEvent()))
  {
    UpdateExpandViewButton();
  }

  if (bucket.HasEvent(ValueChangedEvent(), gs->GetToolbarMode3DModel()))
  {
    UpdateActionButtons();
  }

  if (bucket.HasEvent(ActiveLayerChangeEvent()) || bucket.HasEvent(LayerChangeEvent()) || bucket.HasEvent(MeshContentChangeEvent()))
  {
    UpdateMeshLayerMenu();
  }
}

void
ViewPanel3D::onView3dResize()
{
  UpdateContextButtonLocation();
}

void
ViewPanel3D::onContextMenuUpdateRequested()
{
  // Set up the context menu on the button
  MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
  LayerInspectorDialog *inspector = winmain->GetLayerInspector();

  // Get the corresponding context menu
  auto *ml = m_Model->GetMeshLayers();

  // Create a new menu for the context tool button
  m_ContextButtonMenu->clear();
  auto layer = ml->GetLayer(ml->GetActiveLayerId());
  auto layer_context_menu = layer ? inspector->GetLayerContextMenu(layer) : nullptr;
  if(layer_context_menu)
  {
    auto inspector_actions = inspector->GetLayerContextMenu(layer)->actions();
    for (auto *action : std::as_const(inspector_actions))
      if (action)
      {
        m_ContextButtonMenu->addAction(action);
      }
  }

  // Create a menu for activating another layer
  m_ContextButtonMenu->addSeparator();
  m_ContextButtonMenu->addMenu(m_MeshLayerMenu);
}

void
ViewPanel3D::UpdateContextButtonLocation()
{
  int x = ui->view3d->x() + ui->view3d->width() - m_ContextToolButton->width();
  int y = ui->view3d->y();
  m_ContextToolButton->move(x,y);
}

void
ViewPanel3D::UpdateMeshLayerMenu()
{
  // Fire the signal saying that the context menu needs updating
  emit updateContextMenu();
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
    QMessageBox::warning(this, tr("Problem generating mesh"), IRISexc.what());
    }

  // TODO: Delete this later - should be automatic!
  ui->view3d->repaint();
}

// For coupling mesh layer model with menu
class SelectedMeshLayerDescriptionTraits
{
public:
  using Value = Generic3DModel::SelectedLayerDescriptor;

  static QString GetText(int row, const Value &desc)
  {
    return from_utf8(desc.Name);
  }
  static QIcon GetIcon(int row, const Value &desc)
  {
    return QIcon();
  }
  static QVariant GetIconSignature(int row, const Value &desc)
  {
    return QVariant(0);
  }
};

template <>
class DefaultQMenuRowTraits<unsigned long, SelectedMeshLayerDescriptionTraits::Value>
  : public TextAndIconMenuRowTraits<unsigned long, SelectedMeshLayerDescriptionTraits::Value, SelectedMeshLayerDescriptionTraits>
{};


// For coupling mesh layer model with menu
class FocalPointTargetDescriptionTraits
{
public:
  using Value = Generic3DModel::FocalPointTarget;

  static QString GetText(int row, const Value &desc)
  {
    switch(desc)
    {
      case Generic3DModel::FOCAL_POINT_CURSOR:
        return QCoreApplication::translate("ViewPanel3D", "Cursor position");
      case Generic3DModel::FOCAL_POINT_MAIN_IMAGE_CENTER:
        return QCoreApplication::translate("ViewPanel3D", "Main image center");
      case Generic3DModel::FOCAL_POINT_ACTIVE_MESH_LAYER_CENTER:
        return QCoreApplication::translate("ViewPanel3D", "Active mesh");
        break;
    }
  }
  static QIcon GetIcon(int row, const Value &desc)
  {
    return QIcon();
  }
  static QVariant GetIconSignature(int row, const Value &desc)
  {
    return QVariant(0);
  }
};

template <>
class DefaultQMenuRowTraits<Generic3DModel::FocalPointTarget, FocalPointTargetDescriptionTraits::Value>
  : public TextAndIconMenuRowTraits<Generic3DModel::FocalPointTarget, FocalPointTargetDescriptionTraits::Value, FocalPointTargetDescriptionTraits>
{};


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
  activateOnFlag(m_MeshLayerMenu, m_Model, Generic3DModel::UIF_MULTIPLE_MESHES, QtWidgetActivator::HideInactive);
  activateOnFlag(ui->actionColorBar_Visible, m_Model, Generic3DModel::UIF_MESH_EXTERNAL);

  // Listen to layout events
  connectITK(m_Model->GetParentUI()->GetDisplayLayoutModel(),
             DisplayLayoutModel::ViewPanelLayoutChangeEvent());

  // Listen to changes in active tool to adjust button visibility
  connectITK(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3DModel(),
             ValueChangedEvent());

  // Listen to changes in layer change for 3d view
  makeCoupling(this->m_MeshLayerMenu, m_Model->GetSelectedMeshLayerModel());
  makeCoupling(this->m_FocalPointTargetMenu, m_Model->GetFocalPointTargetModel());

  // Coupling for the color bar enabled state
  makeCoupling(ui->actionColorBar_Visible, m_Model->GetColorBarEnabledByUserModel());

  connectITK(globalUI->GetDriver(), MeshContentChangeEvent());
  connectITK(globalUI->GetDriver(), ActiveLayerChangeEvent());
  connectITK(globalUI->GetDriver(), LayerChangeEvent());
  connectITK(globalUI->GetDriver(), WrapperChangeEvent());
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
    ui->btnExpand->setToolTip(tr("Restore the four-panel display configuration"));
    }
  else
    {
    ui->btnExpand->setIcon(QIcon(":/root/dl_3d.png"));
    ui->btnExpand->setToolTip(tr("Expand the 3D view to occupy the entire window"));
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
    QMessageBox::information(this, tr("No voxels were updated"),
                             tr("The 3D operation did not update any voxels in "
                             "the segmentation. Check that the foreground and "
                             "background labels are selected correctly."));
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

template <unsigned int VDim>
vnl_vector_fixed<double, VDim>
read_json_array(QJsonObject &json, const char *field)
{
  vnl_vector_fixed<double, VDim> vec;
  QString                        error =
    QCoreApplication::translate("ViewPanel3D", "JSON error: %1: expected an array of %2 numbers")
      .arg(field)
      .arg(VDim);

  if(!json[field].isArray() || json[field].toArray().count() != VDim)
    throw error;

  for(unsigned int i = 0; i < VDim; ++i)
  {
    if(!json[field][i].isDouble())
      throw error;
    vec[i] = json[field][i].toDouble();
  }

  return vec;
}

double
read_json_double(QJsonObject &json, const char *field)
{
  QString error = QCoreApplication::translate("ViewPanel3D", "JSON error: %1: expected a number").arg(field);

  if (!json[field].isDouble())
    throw error;

  return json[field].toDouble();
}

bool
read_json_bool(QJsonObject &json, const char *field)
{
  QString error = QCoreApplication::translate("ViewPanel3D", "JSON error: %1: expected a boolean").arg(field);

  if (!json[field].isBool())
    throw error;

  return json[field].isBool();
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

  try
  {
    if (loadDoc.isNull())
      throw tr("JSON error %1").arg(jsonError.errorString());

    QJsonObject json_main = loadDoc.object();

    if(!json_main["file_type"].isString() || json_main["file_type"].toString() != "itksnap_camera_viewpoint")
      throw tr("JSON error: 'file_type' field is missing or malformed");

    if(!json_main["version"].isString())
      throw tr("JSON error: 'version' field is missing");

    if(!json_main["camera"].isObject())
      throw tr("JSON error: 'camera' field is missing or not an object");

    // Read the camera properties
    QJsonObject json_camera = json_main["camera"].toObject();

    CameraState cam;
    cam.position = read_json_array<3>(json_camera, "position");
    cam.focal_point = read_json_array<3>(json_camera, "focal_point");
    cam.view_up = read_json_array<3>(json_camera, "view_up");
    cam.clipping_range = read_json_array<2>(json_camera, "clipping_range");
    cam.view_angle = read_json_double(json_camera, "view_angle");
    cam.parallel_scale = read_json_double(json_camera, "parallel_scale");
    cam.parallel_projection = read_json_bool(json_camera, "parallel_projection");

    m_Model->GetRenderer()->SetCameraState(cam);

    m_Model->GetParentUI()->GetSystemInterface()->GetHistoryManager()->UpdateHistory(
      "CameraViewpoint", file.toStdString(), true);
  }
  catch (QString &error_str)
  {
    QMessageBox::warning(this, tr("Import camera viewpoint error"), error_str);
  }
}

void ViewPanel3D::SaveCameraViewpoint(QString file)
{
  CameraState cam = m_Model->GetRenderer()->GetCameraState();
  QJsonObject json_main;
  QJsonObject json_camera;

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
  json_camera["position"] = camPosition;
  json_camera["focal_point"] = camFocalPoint;
  json_camera["view_up"] = camViewUp;
  json_camera["clipping_range"] = camClippingRange;
  json_camera["view_angle"] = QJsonValue(cam.view_angle);
  json_camera["parallel_scale"] = QJsonValue(cam.parallel_scale);
  json_camera["parallel_projection"] = QJsonValue(cam.parallel_projection).toBool();

  // Main json with header and version
  json_main["file_type"] = QJsonValue("itksnap_camera_viewpoint");
  json_main["version"] = QJsonValue("1.0.0");
  json_main["camera"] = json_camera;

  QFile saveFile(file);
  if (!saveFile.open(QIODevice::WriteOnly)
      || saveFile.write(QJsonDocument(json_main).toJson()) == -1)
    QMessageBox::warning(this, tr("Export camera viewpoint error"),
                         tr("I/O error: %1").arg(saveFile.errorString()));

  m_Model->GetParentUI()->GetSystemInterface()->GetHistoryManager()->UpdateHistory(
    "CameraViewpoint", file.toStdString(), true);
}

void ViewPanel3D::on_actionImport_Viewpoint_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleOpenDialogWithHistory(this,
                                                      m_Model->GetParentUI(),
                                                      "CameraViewpoint",
                                                      tr("Load Camera Viewpoint - ITK-SNAP"),
                                                      tr("Camera Viewpoint JSON File"),
                                                      tr("Camera Files (%1)").arg("*.json"));

  // Open the labels from the selection
  if(selection.length())
    LoadCameraViewpoint(selection);
}

void ViewPanel3D::on_actionExport_Viewpoint_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleSaveDialogWithHistory(
        this, m_Model->GetParentUI(), "CameraViewpoint",
        tr("Save Camera Viewpoint - ITK-SNAP"),
        tr("Camera Viewpoint JSON File"),
        tr("Camera Files (%1)").arg("*.json"),
        true);

  // Open the labels from the selection
  if(selection.length())
    SaveCameraViewpoint(selection);
}

void ViewPanel3D::on_actionContinuous_Update_triggered()
{
  ui->btnUpdateMesh->setVisible(!ui->actionContinuous_Update->isChecked());
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
