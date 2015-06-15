#include "LayerInspectorDialog.h"
#include "ui_LayerInspectorDialog.h"
#include "ContrastInspector.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageWrapper.h"
#include "LayerSelectionModel.h"
#include "GenericImageData.h"
#include "IntensityCurveModel.h"
#include "ColorMapModel.h"
#include "ImageInfoModel.h"
#include "LatentITKEventNotifier.h"
#include "QtSliderCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "LayerGeneralPropertiesModel.h"
#include "SNAPQtCommon.h"
#include "LayerInspectorRowDelegate.h"
#include "QGroupBox"
#include "QComboBox"
#include "QToolBar"
#include <QToolButton>
#include "CollapsableGroupBox.h"
#include "LayerTableRowModel.h"
#include "GlobalUIModel.h"
#include "QtActionCoupling.h"
#include "QtActionGroupCoupling.h"
#include "DisplayLayoutModel.h"

#include <QMenu>

LayerInspectorDialog::LayerInspectorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerInspectorDialog)
{
  ui->setupUi(this);

  // Give the dialog a name for scripting
  this->setObjectName("dlgLayerInspector");

  // Create a toolbar under the layer list
  QToolBar *tb = new QToolBar(this);
  ui->loToolbar->addWidget(tb);
  tb->setIconSize(QSize(16,16));

  m_SaveSelectedButton = new QToolButton(this);

  tb->addAction(ui->actionOpenLayer);
  tb->addWidget(m_SaveSelectedButton);
  tb->addAction(ui->actionLayoutToggle);

  // Set up the action button
  QMenu *menu = new QMenu(this);
  menu->addAction(ui->actionSaveSelectedLayerAs);



  // Set up the list of custom layer widgets
  QVBoxLayout *loLayers = new QVBoxLayout();
  loLayers->setContentsMargins(1, 0, 1, 0);
  loLayers->setSpacing(0);
  ui->saLayersContents->setLayout(loLayers);
  // loLayers->setSizeConstraint(QLayout::SetMinAndMaxSize);
  // loLayers->setContentsMargins(0,0,0,0);
  // loLayers->setSpacing(0);

  // Filter events to the widget containing the list of layers. This is
  // used to hide and show controls (keep UI less busy) and to handle some
  // basic keyboard interaction.
  ui->saLayersContents->installEventFilter(this);
}

LayerInspectorDialog::~LayerInspectorDialog()
{
  delete ui;
}

void LayerInspectorDialog::SetModel(GlobalUIModel *model)
{
  this->m_Model = model;

  // Hook up the model to the component inspectors
  ui->cmpContrast->SetModel(model->GetIntensityCurveModel());
  ui->cmpColorMap->SetModel(model->GetColorMapModel());
  ui->cmpInfo->SetModel(model->GetImageInfoModel());
  ui->cmpMetadata->SetModel(model->GetImageInfoModel());
  ui->cmpComponent->SetModel(model->GetLayerGeneralPropertiesModel());

  // We need to listen to layer changes in the model
  LatentITKEventNotifier::connect(
        model, LayerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // We also need to handle changes to metadata
  LatentITKEventNotifier::connect(
        model->GetDriver(), WrapperMetadataChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // For the tile/stacked button, we currently don't have a coupling mechanism
  // that supports it, so instead, we respond to the event directly
  LatentITKEventNotifier::connect(
        model->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel(),
        ValueChangedEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));


  // Set up the activation on the open layer button
  activateOnFlag(ui->actionOpenLayer, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  activateOnFlag(ui->actionLayoutToggle, m_Model, UIF_MULTIPLE_BASE_LAYERS);
}

void LayerInspectorDialog::SetPageToContrastAdjustment()
{
  ui->tabWidget->setCurrentWidget(ui->cmpContrast);
}

void LayerInspectorDialog::SetPageToColorMap()
{
  ui->tabWidget->setCurrentWidget(ui->cmpColorMap);
}

void LayerInspectorDialog::SetPageToImageInfo()
{
  ui->tabWidget->setCurrentWidget(ui->cmpInfo);
}

QMenu *LayerInspectorDialog::GetLayerContextMenu(ImageWrapperBase *layer)
{
  foreach(LayerInspectorRowDelegate *del, m_Delegates)
    if(del->GetLayer() == layer)
      return del->contextMenu();

  return NULL;
}

QAction *LayerInspectorDialog::GetLayerSaveAction(ImageWrapperBase *layer)
{
  foreach(LayerInspectorRowDelegate *del, m_Delegates)
    if(del->GetLayer() == layer)
      return del->saveAction();

  return NULL;
}

bool LayerInspectorDialog::eventFilter(QObject *source, QEvent *event)
{
  if(source == ui->saLayersContents)
    {
    if(event->type() == QEvent::Enter)
      emit layerListHover(true);
    else if(event->type() == QEvent::Leave)
      emit layerListHover(false);
    }

  return false;
}





void LayerInspectorDialog::GenerateModelsForLayers()
{
  // Iterate over the layers for each class of displayed layers
  LayerIterator it = m_Model->GetDriver()->GetCurrentImageData()->GetLayers(
        MAIN_ROLE |
        OVERLAY_ROLE |
        SNAP_ROLE);

  for(; !it.IsAtEnd(); ++it)
    {
    // Check if a model exists for this layer
    SmartPtr<LayerTableRowModel> model =
        dynamic_cast<LayerTableRowModel *>(
          it.GetLayer()->GetUserData("LayerTableRowModel"));

    // If not, create it and stick as 'user data' into the layer
    if(!model)
      {
      model = LayerTableRowModel::New();
      model->Initialize(m_Model, it.GetLayer());
      it.GetLayer()->SetUserData("LayerTableRowModel", model);
      }
    }
}

void LayerInspectorDialog::BuildLayerWidgetHierarchy()
{
  // Static names for different roles
  static QMap<int, QString> mapRoleNames;
  if(mapRoleNames.size() == 0)
    {
    mapRoleNames[MAIN_ROLE] = "Main Image";
    mapRoleNames[OVERLAY_ROLE] = "Additional Images";
    mapRoleNames[SNAP_ROLE] = "Snake Mode Layers";
    }

  // Get the top-level layout in the pane
  QBoxLayout *lo = (QBoxLayout *) ui->saLayersContents->layout();

  // Before deleting all of the existing widgets in the pane,
  // find the one that is currently selected, and remember its
  // layer, so we can re-select it if needed
  bool have_selected_layer = false, found_selected_layer = false;
  unsigned long selected_layer = 0;
  for(QList<LayerInspectorRowDelegate *>::iterator itdel = m_Delegates.begin();
      itdel != m_Delegates.end(); ++itdel)
    {
    if((*itdel)->selected() && (*itdel)->GetLayer())
      {
      selected_layer = (*itdel)->GetLayer()->GetUniqueId();
      have_selected_layer = true;
      break;
      }
    }

  // Get rid of all existing widgets in the pane
  m_Delegates.clear();
  QLayoutItem * item;
  while ( (item = lo->takeAt( 0 ) ) != NULL )
    {
    if(item->widget())
      {
      delete item->widget();
      }
    delete item;
    }

  // Iterate over the layers for each class of displayed layers
  LayerIterator it = m_Model->GetDriver()->GetCurrentImageData()->GetLayers(
        MAIN_ROLE | OVERLAY_ROLE | SNAP_ROLE);

  // The current role and associated group box
  LayerRole currentRole = NO_ROLE;
  CollapsableGroupBox *currentGroupBox = NULL;

  // The row widget for the main image layer (default selection)
  LayerInspectorRowDelegate *w_main = NULL;

  // Loop over all the layers
  for(; !it.IsAtEnd(); ++it)
    {
    LayerRole role = it.GetRole();
    if(role != currentRole)
      {
      // Create the new groupbox
      currentGroupBox = new CollapsableGroupBox();
      currentRole = role;
      lo->addWidget(currentGroupBox);
      currentGroupBox->setTitle(mapRoleNames[currentRole]);
      }

    // Create the custom widget for this layer
    LayerInspectorRowDelegate *w = new LayerInspectorRowDelegate(this);

    // Is this the main layer? Then remember the widget
    if(role == MAIN_ROLE)
      w_main = w;

    // Find the model for this layer
    SmartPtr<LayerTableRowModel> model =
        dynamic_cast<LayerTableRowModel *>(
          it.GetLayer()->GetUserData("LayerTableRowModel"));

    // Set the model
    w->SetModel(model);

    // Set a name for this widget for debugging purposes
    w->setObjectName(QString().sprintf("wgtRowDelegate_%04d", m_Delegates.size()));

    // Listen to select signals from widget
    connect(w, SIGNAL(selectionChanged(bool)), this, SLOT(layerSelected(bool)));
    connect(w, SIGNAL(contrastInspectorRequested()), this, SLOT(onContrastInspectorRequested()));
    connect(w, SIGNAL(colorMapInspectorRequested()), this, SLOT(onColorMapInspectorRequested()));

    // Select the layer if it was previously selected or nothing was previously
    // selected and the layer is the main layer
    if((have_selected_layer && it.GetLayer()->GetUniqueId() == selected_layer))
      {
      w->setSelected(true);
      found_selected_layer = true;
      }

    else
      {
      w->setSelected(false);
      }

    currentGroupBox->addWidget(w);
    m_Delegates.push_back(w);
    }

  // If we haven't selected anything, select the main layer's widget
  if(!found_selected_layer && w_main)
    w_main->setSelected(true);

  lo->addStretch(1);
}

void LayerInspectorDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(LayerChangeEvent()))
    {
    // Make sure each layer is associated with a model
    // TODO: it would be more attractive to map models to layers somewhere in
    // the 'model layer' rather than in the Qt code. Think about it...
    this->GenerateModelsForLayers();

    // Build the left pane
    this->BuildLayerWidgetHierarchy();

    // TODO: this is the wrong place for this!!!
    m_Model->GetImageInfoModel()->Update();
    m_Model->GetColorMapModel()->Update();
    m_Model->GetIntensityCurveModel()->Update();
    m_Model->GetLayerGeneralPropertiesModel()->Update();
    }

  if(bucket.HasEvent(ValueChangedEvent(),
                     m_Model->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()))
    {
    this->UpdateLayerLayoutAction();
    }
}

void LayerInspectorDialog::layerSelected(bool flag)
{
  // Remove all actions from the save button
  foreach(QAction *action, m_SaveSelectedButton->actions())
    m_SaveSelectedButton->removeAction(action);

  // Turn off all the other widgets
  if(flag)
    {
    // Toggle everything else off
    foreach(LayerInspectorRowDelegate *w, m_Delegates)
      {
      if(w != this->sender())
        w->setSelected(false);
      }

    // Switch the current layer in all the right-pane models
    LayerInspectorRowDelegate *wsel = (LayerInspectorRowDelegate *) this->sender();
    if(!wsel->selected())
      wsel->setSelected(true);
    this->SetActiveLayer(wsel->GetLayer());

    // Put this layer's actions on the menu
    m_SaveSelectedButton->setDefaultAction(wsel->saveAction());
    }
}

void LayerInspectorDialog::onContrastInspectorRequested()
{
  // Make sure the layer is selected
  this->layerSelected(true);
  ui->tabWidget->setCurrentWidget(ui->cmpContrast);

  // Make sure to show the dialog
  this->show();
  this->activateWindow();
  this->raise();
}

void LayerInspectorDialog::onColorMapInspectorRequested()
{
  // Make sure the layer is selected
  this->layerSelected(true);
  ui->tabWidget->setCurrentWidget(ui->cmpColorMap);

  // Make sure to show the dialog
  this->show();
  this->activateWindow();
  this->raise();
}

void LayerInspectorDialog::SetActiveLayer(ImageWrapperBase *layer)
{
  // For our purposes, uninitialized layers are just the same as null layers,
  // they can not participate in layer association.
  if(layer && !layer->IsInitialized())
    layer = NULL;

  // For each model, set the layer
  m_Model->GetColorMapModel()->SetLayer(layer);

  m_Model->GetIntensityCurveModel()->SetLayer(
        layer->GetDisplayMapping()->GetIntensityCurve() ? layer : NULL);

  m_Model->GetImageInfoModel()->SetLayer(layer);
  m_Model->GetLayerGeneralPropertiesModel()->SetLayer(layer);
}

void
LayerInspectorDialog::UpdateLayerLayoutAction()
{
  DisplayLayoutModel *dlm = m_Model->GetDisplayLayoutModel();
  LayerLayout ll = dlm->GetSliceViewLayerLayoutModel()->GetValue();

  if(ll == LAYOUT_TILED)
    {
    ui->actionLayoutToggle->setIcon(QIcon(":/root/layout_thumb_16.png"));
    }
  else if(ll == LAYOUT_STACKED)
    {
    ui->actionLayoutToggle->setIcon(QIcon(":/root/layout_tile_16.png"));
    }
}

void LayerInspectorDialog::on_actionSaveSelectedLayerAs_triggered()
{
  // Save the currently selected layer


}

void LayerInspectorDialog::on_actionLayoutToggle_triggered(bool value)
{
  DisplayLayoutModel *dlm = m_Model->GetDisplayLayoutModel();
  LayerLayout ll = dlm->GetSliceViewLayerLayoutModel()->GetValue();
  if(ll == LAYOUT_TILED)
    {
    dlm->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_STACKED);
    }
  else
    {
    dlm->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_TILED);
    }
}

void LayerInspectorDialog::on_buttonBox_accepted()
{
  this->close();
}

void LayerInspectorDialog::on_buttonBox_rejected()
{
  this->close();
}

void LayerInspectorDialog::advanceTab()
{
  int pos = ui->tabWidget->currentIndex();
  int ntab = ui->tabWidget->count();
  ui->tabWidget->setCurrentIndex((pos + 1) % ntab);
}

void LayerInspectorDialog::on_actionOpenLayer_triggered()
{
  FindUpstreamAction(this, "actionAdd_Overlay")->trigger();
}
