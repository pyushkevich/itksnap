#include "LayerInspectorRowDelegate.h"
#include "ui_LayerInspectorRowDelegate.h"

#include "ImageWrapperBase.h"
#include "LayerTableRowModel.h"
#include "StandaloneMeshWrapper.h"
#include "SNAPQtCommon.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtSliderCoupling.h"
#include "QtActionGroupCoupling.h"
#include "QtActionCoupling.h"
#include "QtSliderCoupling.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFile>
#include <QGuiApplication>
#include <QMenu>
#include <QContextMenuEvent>
#include <QWidgetAction>
#include "QtWidgetActivator.h"
#include "QtCursorOverride.h"
#include "GlobalUIModel.h"
#include "ImageIODelegates.h"
#include "ImageIOWizard.h"
#include "MainImageWindow.h"
#include "MeshExportWizard.h"
#include "SaveModifiedLayersDialog.h"

#include "DisplayMappingPolicy.h"
#include "ColorMap.h"
#include "ColorMapModel.h"

class QAction;


OpacitySliderAction::OpacitySliderAction(QWidget *parent)
  : QWidgetAction(parent)
{
}

QWidget *OpacitySliderAction::createWidget(QWidget *parent)
{
  m_Container = new QWidget(parent);

  m_Slider = new QSlider();
  m_Slider->setOrientation(Qt::Horizontal);

  QHBoxLayout *lo = new QHBoxLayout();
  lo->setContentsMargins(27, 4, 4, 4);
  lo->setSpacing(10);
  lo->addWidget(new QLabel("Opacity:"));
  lo->addWidget(m_Slider);
  m_Container->setLayout(lo);

  return m_Container;
}


QString LayerInspectorRowDelegate::m_SliderStyleSheetTemplate;

LayerInspectorRowDelegate::LayerInspectorRowDelegate(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LayerInspectorRowDelegate)
{
  ui->setupUi(this);

  // Confirugre the popup menu
  m_PopupMenu = new QMenu(this);
  m_PopupMenu->setObjectName("menuPopup");
  m_PopupMenu->setStyleSheet("font-size:11px;");

  // Add the save/close actions
  m_PopupMenu->addAction(ui->actionSave);
  m_PopupMenu->addAction(ui->actionClose);
  m_PopupMenu->addSeparator();
  m_PopupMenu->addAction(ui->actionAutoContrast);
  m_PopupMenu->addAction(ui->actionContrast_Inspector);
  m_PopupMenu->addSeparator();

  // Add the color map menu
  m_ColorMapMenu = m_PopupMenu->addMenu("Color Map");
  m_SystemPresetActionGroup = NULL;

  // Add the component selection menu
  m_DisplayModeMenu = m_PopupMenu->addMenu("Multi-Component Display");
  m_DisplayModeActionGroup = NULL;

  m_PopupMenu->addSeparator();
  m_PopupMenu->addAction(ui->actionPin_layer);
  m_PopupMenu->addAction(ui->actionUnpin_layer);

  // Create a slider for the layer opacity in the context menu
  m_OverlayOpacitySlider = new QSlider(m_PopupMenu);
  m_OverlayOpacitySlider->setOrientation(Qt::Horizontal);
  m_OverlayOpacitySliderAction = new WidgetWithLabelAction(this);
  m_OverlayOpacitySliderAction->setWidget(m_OverlayOpacitySlider);
  m_OverlayOpacitySliderAction->setLabelText("Opacity: ");
  m_PopupMenu->addAction(m_OverlayOpacitySliderAction);

  // Create a menu listing the loaded overlays
  m_OverlaysMenu = m_PopupMenu->addMenu("Overlays");

  // Create a volume rendering menu
  m_VolumeRenderingMenu = m_PopupMenu->addMenu("Volume Rendering");
  m_VolumeRenderingMenu->setObjectName("menuVolRen");
  m_VolumeRenderingMenu->addAction(ui->actionVolumeEnable);
  ui->actionVolumeEnable->setObjectName("actionVolumeEnable");
  m_PopupMenu->addSeparator();

  // Placeholder for image processing commands
  m_PopupMenu->addSeparator();
  QMenu *processMenu = m_PopupMenu->addMenu("Image Processing");
  processMenu->setObjectName("menuProcess");
  processMenu->addAction(ui->actionTextureFeatures);

  // set up an event filter
  ui->inLayerOpacity->installEventFilter(this);

  // Load the style sheet template
  if(!m_SliderStyleSheetTemplate.length())
    {
    QFile qf(":/root/fancyslider.css");
    if(qf.open(QFile::ReadOnly))
      {
      m_SliderStyleSheetTemplate = QString(qf.readAll());
      }
    }

  // Initialize the state
  m_Selected = false;
  m_Hover = false;

  // Paint the row
  UpdateBackgroundPalette();
}

LayerInspectorRowDelegate::~LayerInspectorRowDelegate()
{
  delete ui;
}

void LayerInspectorRowDelegate::SetModel(AbstractLayerTableRowModel *model)
{
  m_Model = model;

  makeCoupling(ui->inLayerOpacity, model->GetLayerOpacityModel());
  makeCoupling(ui->outLayerNickname, model->GetNicknameModel());

  makeCoupling(m_OverlayOpacitySlider, model->GetLayerOpacityModel());
  makeCoupling((QAbstractButton *) ui->btnVisible, model->GetVisibilityToggleModel());
  makeCoupling((QAbstractButton *) ui->btnSticky, model->GetStickyModel());

  if (!model->CheckState(AbstractLayerTableRowModel::UIF_MESH))
    {
    auto image_model = dynamic_cast<ImageLayerTableRowModel*>(model);
    makeCoupling(ui->outComponent, image_model->GetComponentNameModel());
    makeCoupling(ui->actionVolumeEnable, image_model->GetVolumeRenderingEnabledModel());
    }


  const QtWidgetActivator::Options opt_hide = QtWidgetActivator::HideInactive;
  activateOnFlag(ui->actionUnpin_layer, model, AbstractLayerTableRowModel::UIF_UNPINNABLE, opt_hide);
  activateOnFlag(ui->actionPin_layer, model, AbstractLayerTableRowModel::UIF_PINNABLE, opt_hide);
  activateOnAnyFlags(ui->btnSticky, model, AbstractLayerTableRowModel::UIF_UNPINNABLE, AbstractLayerTableRowModel::UIF_PINNABLE, opt_hide);
  activateOnFlag(m_OverlayOpacitySliderAction, model, AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE, opt_hide);
  activateOnFlag(m_ColorMapMenu, model, AbstractLayerTableRowModel::UIF_COLORMAP_ADJUSTABLE, opt_hide);
  activateOnFlag(m_DisplayModeMenu, model, AbstractLayerTableRowModel::UIF_MULTICOMPONENT, opt_hide);
  activateOnFlag(ui->outComponent, model, AbstractLayerTableRowModel::UIF_MULTICOMPONENT, opt_hide);

  // makeActionVisibilityCoupling(ui->actionUnpin_layer, model->GetStickyModel());
  // makeActionVisibilityCoupling(ui->actionPin_layer, model->GetStickyModel(), true);
  // makeActionVisibilityCoupling(m_OverlayOpacitySliderAction, model->GetStickyModel());

  // Hook up some activations
  activateOnFlag(ui->btnVisible, model, AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE, opt_hide);
  activateOnFlag(ui->inLayerOpacity, model, AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE, opt_hide);
  // activateOnFlag(ui->btnMoveUp, model, AbstractLayerTableRowModel::UIF_MOVABLE_UP);
  // activateOnFlag(ui->btnMoveDown, model, AbstractLayerTableRowModel::UIF_MOVABLE_DOWN);
  activateOnFlag(ui->actionClose, model, AbstractLayerTableRowModel::UIF_CLOSABLE);
  activateOnFlag(ui->actionSave, model, AbstractLayerTableRowModel::UIF_SAVABLE);
  activateOnFlag(ui->actionAutoContrast, model, AbstractLayerTableRowModel::UIF_CONTRAST_ADJUSTABLE);
  activateOnFlag(m_VolumeRenderingMenu, model, AbstractLayerTableRowModel::UIF_VOLUME_RENDERABLE, opt_hide);
  activateOnFlag(m_PopupMenu->findChild<QMenu*>("menuProcess"), model, AbstractLayerTableRowModel::UIF_IMAGE, opt_hide);
  activateOnFlag(m_OverlaysMenu, model, AbstractLayerTableRowModel::UIF_IMAGE, opt_hide);

  // Hook up the colormap and the slider's style sheet
  connectITK(m_Model->GetLayer(), WrapperChangeEvent());
  OnNicknameUpdate();
  ApplyColorMap();

  // Listen to changes in all layer organization and metadata, as this affects the list
  // of overlays shown in the context menu
  connectITK(m_Model->GetParentModel()->GetDriver(), LayerChangeEvent());
  connectITK(m_Model->GetParentModel()->GetDriver(), WrapperMetadataChangeEvent());

  // Listen to preset changes from the color map model
  connectITK(m_Model->GetParentModel()->GetColorMapModel(),
             ColorMapModel::PresetUpdateEvent());

  // Listen to changes in the currently selected layer in GlobalState
  connectITK(m_Model->GetParentModel()->GetGlobalState()->GetSelectedLayerIdModel(),
              ValueChangedEvent());

  // Listen to changes in the currently selected layer in GlobalState
  connectITK(m_Model->GetParentModel()->GetGlobalState()->GetSelectedSegmentationLayerIdModel(),
              ValueChangedEvent());

  // Update the color map menu
  UpdateColorMapMenu();

  // Update the component menu
  UpdateComponentMenu();

  // Update the overlays
  this->UpdateOverlaysMenu();

  // Update the appearance
  this->UpdateTextFont();
}

WrapperBase *LayerInspectorRowDelegate::GetLayer() const
{
  // No model? No layer.
  if(!m_Model)
    return NULL;

  // Must update the model, because the layer might have been deleted
  // and we must make sure that we return a clean layer
  m_Model->Update();
  return m_Model->GetLayer();
}

void LayerInspectorRowDelegate::UpdateBackgroundPalette()
{
  // The color scheme
  QColor scheme_light[] = {
    QColor(180,180,215), QColor(190,190,225), QColor(225,225,235), QColor(235,235,235)
  };

  QColor scheme_dark[] = {
    QColor(115,115,160), QColor(105,105,150), QColor(70,70,70), QColor(60,60,60)
  };

  // Get the current palette
  QPalette palette = QGuiApplication::palette();

  // Determine if the window color is dark or bright
  bool dark_scheme = palette.color(QPalette::Window).valueF() < 0.5;
  QColor* scheme = dark_scheme ? scheme_dark : scheme_light;

  // Set the first color in the gradient
  QColor stop_1, stop_2;
  if(m_Selected && m_Hover)
    stop_1 = scheme[0];
  else if(m_Selected)
    stop_1 = scheme[1];
  else if(m_Hover)
    stop_1 = scheme[2];
  else
    stop_1 = scheme[3];

  // Set the second color as the offset
  stop_2 = dark_scheme ? stop_1.lighter(120) : stop_1.lighter(120);
  QLinearGradient linearGradient(QPointF(0, 0), QPointF(0, this->height()));
  linearGradient.setColorAt(0, stop_1);
  linearGradient.setColorAt(1, stop_2);

  palette.setBrush(QPalette::Window, QBrush(linearGradient));
  ui->frame->setPalette(palette);
}

void LayerInspectorRowDelegate::UpdateTextFont()
{
  // Whether the row is shown in bold does not have to do with click-selection in the table, but
  // with whether the layer is the currently selected anatomical layer or the currently selected
  // segmentation layer.
  bool activated = m_Model && m_Model->IsActivated();

  // Also set the font for the label
  if(ui->outLayerNickname->font().bold() != activated)
    {
    QFont font = ui->outLayerNickname->font();
    font.setBold(activated);
    ui->outLayerNickname->setFont(font);
    }
}

void LayerInspectorRowDelegate::setSelected(bool value)
{
  auto mesh_model = dynamic_cast<MeshLayerTableRowModel*>(m_Model.GetPointer());

  if(m_Selected != value)
    {
    m_Selected = value;

    // Update selection in the model
    m_Model->SetActivated(value);

    // Notify other delegates to change
    emit selectionChanged(value);

    // Update the look and feel
    this->UpdateBackgroundPalette();
    this->UpdateTextFont();

    // Update!
    this->update();
    }
  else if (mesh_model)
    {
    // Remove bold label from mesh that is no longer active
    // -- this happens when a mesh row is "activated" but not "selected" by
    // -- selection of a related image layer. And then if another mesh layer
    // -- is selected, we need this logic to "de-activate" current layer,
    // -- because "m_Selected" is still false and the argument "value" is false
    this->UpdateTextFont();
    }

}

QAction *LayerInspectorRowDelegate::saveAction() const
{
  return ui->actionSave;
}

QAction *LayerInspectorRowDelegate::closeAction() const
{
  return ui->actionClose;
}

QMenu *LayerInspectorRowDelegate::contextMenu() const
{
  return this->m_PopupMenu;
}

void LayerInspectorRowDelegate::enterEvent(QEnterEvent *)
{
  m_Hover = true;
  this->UpdateBackgroundPalette();
}

void LayerInspectorRowDelegate::leaveEvent(QEvent *)
{
  m_Hover = false;
  this->UpdateBackgroundPalette();
}

void LayerInspectorRowDelegate::mousePressEvent(QMouseEvent *)
{
  this->setSelected(true);
}

void LayerInspectorRowDelegate::mouseReleaseEvent(QMouseEvent *)
{
  this->setSelected(true);
}

void LayerInspectorRowDelegate::contextMenuEvent(QContextMenuEvent *evt)
{
  m_PopupMenu->popup(evt->globalPos());
}

bool LayerInspectorRowDelegate::eventFilter(QObject *, QEvent *evt)
{
  if(evt->type() == QEvent::FocusIn)
    {
    if(!this->selected())
      {
      this->setSelected(true);
      return true;
      }
    }

  if(evt->type() == QEvent::MouseButtonPress)
    {
    if(!this->selected())
      this->setSelected(true);
    return false;
    }


  return false;
}

void LayerInspectorRowDelegate::UpdateColorMapMenu()
{
  // The presets are available from the color map model. We can use them
  // regardless of the row that is currently selected
  ColorMapModel *cmm = m_Model->GetParentModel()->GetColorMapModel();
  ColorMapPresetManager *pm = cmm->GetPresetManager();

  // Get the system and user presets
  ColorMapModel::PresetList pSystem, pUser;
  cmm->GetPresets(pSystem, pUser);

  // Remove all of the existing actions
  m_ColorMapMenu->clear();
  if(m_SystemPresetActionGroup)
    delete m_SystemPresetActionGroup;

  // Create a map from preset names to actions
  std::map<std::string, QAction *> actionMap;

  // Add all of the system presets
  m_SystemPresetActionGroup = new QActionGroup(this);

  // Create the actions for the system presets
  for(unsigned int i = 0; i < pSystem.size(); i++)
    {
    QIcon icon = CreateColorMapIcon(16, 16, pm->GetPreset(pSystem[i]));
    QAction *action = m_SystemPresetActionGroup->addAction(icon, from_utf8(pSystem[i]));
    action->setCheckable(true);
    actionMap[pSystem[i]] = action;
    }

  // Add a separator to the action group
  m_SystemPresetActionGroup->addAction("")->setSeparator(true);

  // Add the user presets to the action group
  for(unsigned int i = 0; i < pUser.size(); i++)
    {
    QIcon icon = CreateColorMapIcon(16, 16, pm->GetPreset(pUser[i]));
    QAction *action = m_SystemPresetActionGroup->addAction(icon, from_utf8(pUser[i]));
    action->setCheckable(true);
    actionMap[pUser[i]] = action;
    }

  // Connect the action group to the model
  makeActionGroupCoupling(m_SystemPresetActionGroup,
                          actionMap,
                          m_Model->GetColorMapPresetModel());

  // Add the action group to the menu
  m_ColorMapMenu->addActions(m_SystemPresetActionGroup->actions());

  // Add the link to color map editor
  m_ColorMapMenu->addSeparator();
  m_ColorMapMenu->addAction(ui->actionColor_Map_Editor);
}

void LayerInspectorRowDelegate::UpdateComponentMenu()
{
  m_DisplayModeMenu->clear();

  if(m_DisplayModeActionGroup)
    delete m_DisplayModeActionGroup;

  // No action taken for mesh layers
  if (m_Model->CheckState(AbstractLayerTableRowModel::UIF_MESH))
    return;

  auto image_model = dynamic_cast<ImageLayerTableRowModel*>(m_Model.GetPointer());

  // Create a map from display modes to actions
  std::map<MultiChannelDisplayMode, QAction *> actionMap;

  // Add all of the system presets
  m_DisplayModeActionGroup = new QActionGroup(this);

  // Get the list of all available display modes from the model
  auto &modes = image_model->GetAvailableDisplayModes();

  // Create the actions for the display modes
  ImageLayerTableRowModel::DisplayModeList::const_iterator it;
  for(it = modes.begin(); it != modes.end(); it++)
    {
    MultiChannelDisplayMode mode = *it;

    // Insert some separators into the menu
    if(mode.SelectedScalarRep == SCALAR_REP_MAGNITUDE || mode.UseRGB)
      m_DisplayModeMenu->addSeparator();

    // Create an action
    QAction *action = m_DisplayModeMenu->addAction(
          from_utf8(image_model->GetDisplayModeString(mode)));

    action->setCheckable(true);

    m_DisplayModeActionGroup->addAction(action);

    actionMap[mode] = action;
    }

  // Hook up with the ctions
  makeActionGroupCoupling(m_DisplayModeActionGroup, actionMap,
                          image_model->GetDisplayModeModel());
}

#include "GenericImageData.h"
#include "LayerInspectorDialog.h"

void LayerInspectorRowDelegate::UpdateOverlaysMenu()
{
  int k = 0;

  // Clear the overlays menu
  m_OverlaysMenu->clear();

  // Do nothing for mesh layer
  if (m_Model->CheckState(AbstractLayerTableRowModel::UIF_MESH))
    return;

  auto image_layer = dynamic_cast<ImageWrapperBase*>(m_Model->GetLayer());

  if (!image_layer)
    return;

  // If the current layer is sticky, disable the menu
  if(m_Model->GetLayer() && !image_layer->IsSticky())
    {
    // Get the current image data
    GenericImageData *gid = m_Model->GetParentModel()->GetDriver()->GetCurrentImageData();

    // Find the number of sticky layers
    for(LayerIterator it = gid->GetLayers(OVERLAY_ROLE | SNAP_ROLE); !it.IsAtEnd(); ++it)
      {
      if(it.GetLayer()->IsSticky())
        {
        // Find the context menu for that layer
        LayerInspectorDialog *insp = findParentWidget<LayerInspectorDialog>(this);
        QMenu *menu = insp->GetLayerContextMenu(it.GetLayer());
        if(menu)
          {
          m_OverlaysMenu->addAction(menu->menuAction());
          k++;
          }
        }
      }
    }

  // Enable overlays menu if it's not empty
  m_OverlaysMenu->menuAction()->setVisible(k > 0);
}


void LayerInspectorRowDelegate::OnNicknameUpdate()
{
  const char *layer_type =
      (dynamic_cast<MeshLayerTableRowModel*>(m_Model.GetPointer())) ? "mesh" : "image";

  // Update things that depend on the nickname
  QString name = from_utf8(m_Model->GetNickname());
  ui->actionSave->setText(QString("Save %1 \"%2\" ...").arg(layer_type).arg(name));
  ui->actionSave->setToolTip(ui->actionSave->text());
  ui->actionClose->setText(QString("Close %1 \"%2\"").arg(layer_type).arg(name));
  ui->actionClose->setToolTip(ui->actionClose->text());
  ui->outLayerNickname->setToolTip(name);

  m_PopupMenu->setTitle(name);
}

void LayerInspectorRowDelegate::onModelUpdate(const EventBucket &bucket)
{
  IRISApplication *app = m_Model->GetParentModel()->GetDriver();
  GlobalState *gs = app->GetGlobalState();
  if(bucket.HasEvent(WrapperDisplayMappingChangeEvent()))
    {
    this->ApplyColorMap();
    }
  if(bucket.HasEvent(WrapperMetadataChangeEvent(), m_Model->GetLayer()))
    {
    this->OnNicknameUpdate();
    }
  if(bucket.HasEvent(ColorMapModel::PresetUpdateEvent()))
    {
    this->UpdateColorMapMenu();
    }
  if(bucket.HasEvent(LayerChangeEvent()) || bucket.HasEvent(WrapperChangeEvent()))
    {
    this->UpdateOverlaysMenu();
    this->UpdateTextFont();
    }
  if(bucket.HasEvent(ValueChangedEvent(), gs->GetSelectedLayerIdModel()))
    {
    unsigned long sid = gs->GetSelectedLayerId();
    this->setSelected(m_Model->GetLayer() && sid == m_Model->GetLayer()->GetUniqueId());
    this->UpdateTextFont();
    }
  if(bucket.HasEvent(ValueChangedEvent(), m_Model->GetParentModel()->GetGlobalState()->GetSelectedSegmentationLayerIdModel()))
    {
    unsigned long sid = gs->GetSelectedSegmentationLayerId();
    this->setSelected(m_Model->GetLayer() && sid == m_Model->GetLayer()->GetUniqueId());
    this->UpdateTextFont();
    }

}

void LayerInspectorRowDelegate::mouseMoveEvent(QMouseEvent *)
{
  this->setSelected(true);
}

void LayerInspectorRowDelegate::ApplyColorMap()
{
  ColorMap *cm = m_Model->GetLayer()->GetDisplayMapping()->GetColorMap();
  if(cm)
    {
    QStringList stops;
    for(size_t i = 0; i < cm->GetNumberOfCMPoints(); i++)
      {
      ColorMap::CMPoint cmp = cm->GetCMPoint(i);
      for(int side = 0; side < 2; side++)
        {
        if((i == 0 && side == 0) ||
           (i == cm->GetNumberOfCMPoints()-1 && side == 1) ||
           (cmp.m_Type == ColorMap::CONTINUOUS && side == 1))
          continue;

        QString cmstr = QString("stop: %1 rgba(%2, %3, %4, %5)")
            .arg(cmp.m_Index)
            .arg(cmp.m_RGBA[side][0]).arg(cmp.m_RGBA[side][1])
            .arg(cmp.m_RGBA[side][2]).arg(cmp.m_RGBA[side][3]);
        stops << cmstr;
        }
      }
    QString gradient = QString("qlineargradient(x1:0, y1:0, x2:1, y2:0, %1);")
        .arg(stops.join(","));

    QString stylesheet = m_SliderStyleSheetTemplate;
    stylesheet.replace("#gradient#", gradient);
    ui->inLayerOpacity->setStyleSheet(stylesheet);
    }
}

void LayerInspectorRowDelegate::on_btnMenu_pressed()
{
  m_PopupMenu->popup(QCursor::pos());
  ui->btnMenu->setDown(false);
}

/*
void LayerInspectorRowDelegate::on_btnMoveUp_clicked()
{
  m_Model->MoveLayerUp();
}

void LayerInspectorRowDelegate::on_btnMoveDown_pressed()
{
  m_Model->MoveLayerDown();
}
*/

void LayerInspectorRowDelegate::on_actionSave_triggered()
{
  auto mesh_model = dynamic_cast<MeshLayerTableRowModel*>(m_Model.GetPointer());
  if (mesh_model)
    {
    if (mesh_model->CheckState(AbstractLayerTableRowModel::UIF_SAVABLE))
      {
      MeshExportWizard wizard(this);
      wizard.SetModel(m_Model->GetParentModel()->GetMeshExportModel());
      wizard.exec();
      }
    return;
    }

  auto image_model = dynamic_cast<ImageLayerTableRowModel*>(m_Model.GetPointer());
  if (!image_model)
    return;

  // Create a model for IO
  SmartPtr<ImageIOWizardModel> model = image_model->CreateIOWizardModelForSave();

  // Interactive
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void LayerInspectorRowDelegate::on_actionClose_triggered()
{
  bool do_close = false;

  // Mesh specific logic
  if (m_Model->CheckState(AbstractLayerTableRowModel::UIF_MESH))
    {
    do_close = true;
    }
  else
    {
    // Should we prompt for a single layer or all layers?
    ImageWrapperBase *prompted_layer = nullptr;

    if (!m_Model->IsMainLayer())
      prompted_layer = dynamic_cast<ImageWrapperBase*>(m_Model->GetLayer());

    // Prompt for changes
    if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParentModel(), prompted_layer))
      {
      do_close = true;
      }
    }

  if (do_close)
    m_Model->CloseLayer();
}

void LayerInspectorRowDelegate::onColorMapPresetSelected()
{
}

void LayerInspectorRowDelegate::on_actionAutoContrast_triggered()
{
  m_Model->AutoAdjustContrast();
}

void LayerInspectorRowDelegate::on_actionTextureFeatures_triggered()
{
  QtCursorOverride c(Qt::WaitCursor);

  // Do nothing for mesh layer
  if (m_Model->CheckState(AbstractLayerTableRowModel::UIF_MESH))
    return;

  auto image_model = dynamic_cast<ImageLayerTableRowModel*>(m_Model.GetPointer());
  if (image_model)
    image_model->GenerateTextureFeatures();
}

void LayerInspectorRowDelegate::on_actionPin_layer_triggered()
{
  m_Model->SetSticky(true);
}

void LayerInspectorRowDelegate::on_actionUnpin_layer_triggered()
{
  m_Model->SetSticky(false);
}

void LayerInspectorRowDelegate::on_actionContrast_Inspector_triggered()
{
  emit contrastInspectorRequested();
}



WidgetWithLabelAction::WidgetWithLabelAction(QWidget *parent)
  : QWidgetAction(parent)
{
  m_Container = new QWidget(parent);
  m_Label = new QLabel();

  QHBoxLayout *lo = new QHBoxLayout();
  lo->setContentsMargins(27, 4, 4, 4);
  lo->setSpacing(10);
  lo->addWidget(m_Label);
  m_Container->setLayout(lo);

  this->setDefaultWidget(m_Container);

  connect(this, SIGNAL(changed()), this, SLOT(onChanged()));
}

void WidgetWithLabelAction::setWidget(QWidget *widget)
{
  m_Container->layout()->addWidget(widget);
}

void WidgetWithLabelAction::setLabelText(const QString &text)
{
  m_Label->setText(text);
}

void WidgetWithLabelAction::onChanged()
{
  m_Container->setVisible(this->isVisible());
  m_Container->setEnabled(this->isEnabled());
}

void LayerInspectorRowDelegate::on_actionColor_Map_Editor_triggered()
{
  emit colorMapInspectorRequested();
}
