#include "ViewPanel3D.h"
#include "LayerInspectorDialog.h"
#include "ui_ViewPanel3D.h"
#include "GlobalUIModel.h"
#include "Generic3DModel.h"
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

  // Listen for leave/enter events to active context button
  connect(ui->view3d, SIGNAL(mouseEntered()), m_ContextToolButton, SLOT(show()));
  connect(ui->view3d, SIGNAL(mouseLeft()), m_ContextToolButton, SLOT(hide()));
  connect(ui->view3d, SIGNAL(resized()), this, SLOT(onView3dResize()));

  // Create the focal point target submenu
  m_FocalPointTargetMenu = new QMenu(tr("Focus Camera on "), this);

  // Set up the camera context menu
  m_DropMenu = new QMenu(this);
  m_DropMenu->setStyleSheet("font-size:11px;");
  m_DropMenu->addAction(ui->actionReset_Viewpoint);
  m_DropMenu->addMenu(m_FocalPointTargetMenu);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionSave_Viewpoint);
  m_DropMenu->addAction(ui->actionRestore_Viewpoint);
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionContinuous_Update);
  ui->actionContinuous_Update->setObjectName("actionContinuousUpdate");
  m_DropMenu->addSeparator();
  m_DropMenu->addAction(ui->actionClear_Rendering);
  m_DropMenu->addSeparator();

  // Menu for listing layers
  m_MeshLayerMenu = new QMenu(tr("Active Mesh"), this);
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

  if (bucket.HasEvent(ActiveLayerChangeEvent()))
  {
    UpdateMeshLayerMenu();
  }

  if (bucket.HasEvent(LayerChangeEvent()))
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
  QMenu *new_menu = new QMenu(m_ContextToolButton);
  new_menu->setStyleSheet("font-size:11px;");
  auto inspector_actions = inspector->GetLayerContextMenu(ml->GetLayer(ml->GetActiveLayerId()))->actions();
  for (auto *action : std::as_const(inspector_actions))
    if(action)
      new_menu->addAction(action);
  m_ContextToolButton->setMenu(new_menu);

  // Create a menu for activating another layer
  new_menu->addSeparator();
  new_menu->addMenu(m_MeshLayerMenu);
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

/*
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
*/

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
