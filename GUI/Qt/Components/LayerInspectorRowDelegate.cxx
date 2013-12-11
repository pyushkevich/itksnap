#include "LayerInspectorRowDelegate.h"
#include "ui_LayerInspectorRowDelegate.h"

#include "ImageWrapperBase.h"
#include "LayerTableRowModel.h"
#include "SNAPQtCommon.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtSliderCoupling.h"
#include "QtActionGroupCoupling.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QFile>
#include <QMenu>
#include <QContextMenuEvent>
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include "ImageIODelegates.h"
#include "ImageIOWizard.h"
#include "MainImageWindow.h"
#include "SaveModifiedLayersDialog.h"

#include "DisplayMappingPolicy.h"
#include "ColorMap.h"
#include "ColorMapModel.h"

class QAction;

QString LayerInspectorRowDelegate::m_SliderStyleSheetTemplate;

LayerInspectorRowDelegate::LayerInspectorRowDelegate(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LayerInspectorRowDelegate)
{
  ui->setupUi(this);

  // Confirugre the popup menu
  m_PopupMenu = new QMenu(this);
  m_PopupMenu->setStyleSheet("font-size:11px;");

  // Add the save/close actions
  m_PopupMenu->addAction(ui->actionSave);
  m_PopupMenu->addAction(ui->actionClose);
  m_PopupMenu->addSeparator();
  m_PopupMenu->addAction(ui->actionAutoContrast);

  // Add the color map menu
  m_ColorMapMenu = m_PopupMenu->addMenu("Color Map");
  m_SystemPresetActionGroup = NULL;

  // Add the component selection menu
  m_DisplayModeMenu = m_PopupMenu->addMenu("Multi-Component Display");
  m_DisplayModeActionGroup = NULL;

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
  ui->stack->setCurrentWidget(ui->pageBlank);
  UpdateBackgroundPalette();
}

LayerInspectorRowDelegate::~LayerInspectorRowDelegate()
{
  delete ui;
}

void LayerInspectorRowDelegate::SetModel(LayerTableRowModel *model)
{
  m_Model = model;

  makeCoupling(ui->inLayerOpacity, model->GetLayerOpacityModel());
  makeCoupling(ui->outLayerNickname, model->GetNicknameModel());
  makeCoupling(ui->outComponent, model->GetComponentNameModel());

  makeCoupling((QAbstractButton *) ui->btnSticky, model->GetStickyModel());
  makeCoupling((QAbstractButton *) ui->btnVisible, model->GetVisibilityToggleModel());

  // Hook up some activations
  activateOnFlag(ui->btnVisible, model, LayerTableRowModel::UIF_OPACITY_EDITABLE);
  activateOnFlag(ui->inLayerOpacity, model, LayerTableRowModel::UIF_OPACITY_EDITABLE);
  activateOnFlag(ui->btnSticky, model, LayerTableRowModel::UIF_PINNABLE);
  activateOnFlag(ui->btnMoveUp, model, LayerTableRowModel::UIF_MOVABLE_UP);
  activateOnFlag(ui->btnMoveDown, model, LayerTableRowModel::UIF_MOVABLE_DOWN);
  activateOnFlag(ui->actionClose, model, LayerTableRowModel::UIF_CLOSABLE);
  activateOnFlag(ui->actionAutoContrast, model, LayerTableRowModel::UIF_CONTRAST_ADJUSTABLE);
  activateOnFlag(m_ColorMapMenu, model, LayerTableRowModel::UIF_COLORMAP_ADJUSTABLE);
  activateOnFlag(m_DisplayModeMenu, model, LayerTableRowModel::UIF_MULTICOMPONENT);

  // Hook up the colormap and the slider's style sheet
  connectITK(m_Model->GetLayer(), WrapperChangeEvent());
  OnNicknameUpdate();
  ApplyColorMap();

  // Listen to preset changes from the color map model
  connectITK(m_Model->GetParentModel()->GetColorMapModel(),
             ColorMapModel::PresetUpdateEvent());

  // Update the color map menu
  UpdateColorMapMenu();

  // Update the component menu
  UpdateComponentMenu();

  // The page shown in this widget depends on whether the visibility editing
  // is on or off
  connectITK(m_Model->GetParentModel()->GetLayerVisibilityEditableModel(),
             ValueChangedEvent());
  UpdateVisibilityControls();
}

ImageWrapperBase *LayerInspectorRowDelegate::GetLayer() const
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
  // Set up a pallete for the background
  QPalette* palette = new QPalette();
  QLinearGradient linearGradient(QPointF(0, 0), QPointF(0, this->height()));

  if(m_Selected && m_Hover)
    {
    linearGradient.setColorAt(0, QColor(180,180,215));
    linearGradient.setColorAt(1, QColor(200,200,235));
    }
  else if(m_Selected)
    {
    linearGradient.setColorAt(0, QColor(190,190,225));
    linearGradient.setColorAt(1, QColor(210,210,245));
    }
  else if(m_Hover)
    {
    linearGradient.setColorAt(0, QColor(225,225,235));
    linearGradient.setColorAt(1, QColor(245,245,255));
    }
  else
    {
    linearGradient.setColorAt(0, QColor(235,235,235));
    linearGradient.setColorAt(1, QColor(255,255,255));
    }

  palette->setBrush(QPalette::Window, *(new QBrush(linearGradient)));
  ui->frame->setPalette(*palette);

  // Also set the font for the label
  if(ui->outLayerNickname->font().bold() != m_Selected)
    {
    QFont font = ui->outLayerNickname->font();
    font.setBold(m_Selected);
    ui->outLayerNickname->setFont(font);
    }
}

void LayerInspectorRowDelegate::setSelected(bool value)
{
  if(m_Selected != value)
    {
    m_Selected = value;
    emit selectionChanged(value);

    // Update the look and feel
    this->UpdateBackgroundPalette();
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

void LayerInspectorRowDelegate::enterEvent(QEvent *)
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

void LayerInspectorRowDelegate::UpdateVisibilityControls()
{  
  if(m_Model->GetParentModel()->GetLayerVisibilityEditable())
    {
    ui->stack->setCurrentWidget(ui->pageControls);
    }
  else
    {
    ui->stack->setCurrentWidget(ui->pageBlank);
    }
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
}

void LayerInspectorRowDelegate::UpdateComponentMenu()
{
  m_DisplayModeMenu->clear();

  if(m_DisplayModeActionGroup)
    delete m_DisplayModeActionGroup;

  // Create a map from display modes to actions
  std::map<MultiChannelDisplayMode, QAction *> actionMap;

  // Add all of the system presets
  m_DisplayModeActionGroup = new QActionGroup(this);

  // Get the list of all available display modes from the model
  const LayerTableRowModel::DisplayModeList &modes = m_Model->GetAvailableDisplayModes();

  // Create the actions for the display modes
  LayerTableRowModel::DisplayModeList::const_iterator it;
  for(it = modes.begin(); it != modes.end(); it++)
    {
    MultiChannelDisplayMode mode = *it;

    // Insert some separators into the menu
    if(mode.SelectedScalarRep == SCALAR_REP_MAGNITUDE || mode.UseRGB)
      m_DisplayModeMenu->addSeparator();

    // Create an action
    QAction *action = m_DisplayModeMenu->addAction(
          from_utf8(m_Model->GetDisplayModeString(mode)));

    action->setCheckable(true);

    m_DisplayModeActionGroup->addAction(action);

    actionMap[mode] = action;
    }

  // Hook up with the ctions
  makeActionGroupCoupling(m_DisplayModeActionGroup, actionMap,
                          m_Model->GetDisplayModeModel());
}


void LayerInspectorRowDelegate::OnNicknameUpdate()
{
  // Update things that depend on the nickname
  QString name = from_utf8(m_Model->GetNickname());
  ui->actionSave->setText(QString("Save image \"%1\" ...").arg(name));
  ui->actionSave->setToolTip(ui->actionSave->text());
  ui->actionClose->setText(QString("Close image \"%1\"").arg(name));
  ui->actionClose->setToolTip(ui->actionClose->text());
  ui->outLayerNickname->setToolTip(name);

}

void LayerInspectorRowDelegate::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(WrapperDisplayMappingChangeEvent()))
    {
    this->ApplyColorMap();
    }
  if(bucket.HasEvent(WrapperMetadataChangeEvent()))
    {
    this->OnNicknameUpdate();
    }
  if(bucket.HasEvent(ValueChangedEvent()))
    {
    this->UpdateVisibilityControls();
    }
  if(bucket.HasEvent(ColorMapModel::PresetUpdateEvent()))
    {
    this->UpdateColorMapMenu();
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
    for(int i = 0; i < cm->GetNumberOfCMPoints(); i++)
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

void LayerInspectorRowDelegate::on_btnMoveUp_clicked()
{
  m_Model->MoveLayerUp();
}

void LayerInspectorRowDelegate::on_btnMoveDown_pressed()
{
  m_Model->MoveLayerDown();
}

void LayerInspectorRowDelegate::on_actionSave_triggered()
{
  // Create a model for IO
  SmartPtr<ImageIOWizardModel> model = m_Model->CreateIOWizardModelForSave();

  // Interactive
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void LayerInspectorRowDelegate::on_actionClose_triggered()
{
  // Should we prompt for a single layer or all layers?
  ImageWrapperBase *prompted_layer = m_Model->IsMainLayer() ? NULL : m_Model->GetLayer();

  // Prompt for changes
  if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParentModel(), prompted_layer))
    {
    m_Model->CloseLayer();
    }
}

void LayerInspectorRowDelegate::onColorMapPresetSelected()
{
}

void LayerInspectorRowDelegate::on_actionAutoContrast_triggered()
{
  m_Model->AutoAdjustContrast();
}
