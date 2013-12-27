#include "MainControlPanel.h"
#include "ui_MainControlPanel.h"
#include "GlobalUIModel.h"
#include "QtWidgetActivator.h"
#include "SNAPQtCommon.h"
#include "LabelSelectionButton.h"
#include "LabelSelectionPopup.h"
#include "MainImageWindow.h"
#include "GlobalState.h"

#include <QtActionGroupCoupling.h>
#include <QActionGroup>
#include <QMenu>

/*
 * A style sheet for making blue buttons with a drop down. For now we are
 * not going to use these, but may need them if more tools are added to
 * the main toolbox

    QToolButton {
    border-image: url(:/root/fltkbutton.png) repeat;
    border-top-width: 8px;
    border-bottom-width: 8px;
    border-left-width: 3px;
    border-right-width: 3px;
    background-origin: content;
    padding-top: -8px;
    padding-bottom: -8px;
    padding-left: -7px;
    padding-right: 1px;
    width:33px;
    height:28px;
    }

    QToolButton[popupMode="0"]
    {
    width:28px;
    padding-left: -2px;
    padding-right: -2px;

    }

    QToolButton:pressed, QToolButton:checked {
      border-image: url(:/root/fltkbutton_pressed.png) repeat;
    }

    QToolButton::menu-button {
    border:0px;
    margin:0px;
    padding-top: 17px;
    padding-right: 2px;
    }

    QToolButton::menu-arrow {
    image: url(:/root/menu-arrow.png);
    }

    */

/*
 * Some commented out code for setting up a dropdown menu of actions

  // Create an action group for the navigation button
  QMenu *menuNav = new QMenu(this);
  menuNav->addAction(ui->actionCrosshair);
  menuNav->addAction(ui->actionZoomPan);
  ui->btnNavigation->setMenu(menuNav);
  ui->btnNavigation->setDefaultAction(ui->actionCrosshair);

  connect(menuNav, SIGNAL(triggered(QAction*)),
          this, SLOT(onNavigationButtonAction(QAction *)));

  // Create an action group for the drawing tool button
  QMenu *menuDraw = new QMenu(this);
  menuDraw->addAction(ui->actionPolygon);
  menuDraw->addAction(ui->actionPaintbrush);
  ui->btnDrawing->setMenu(menuDraw);
  ui->btnDrawing->setDefaultAction(ui->actionPolygon);

  connect(menuDraw, SIGNAL(triggered(QAction*)),
          this, SLOT(onDrawingButtonAction(QAction *)));

 */

#include <QToolBar>
#include <QWhatsThis>

MainControlPanel::MainControlPanel(MainImageWindow *parent) :
  SNAPComponent(parent),
  ui(new Ui::MainControlPanel)
{
  ui->setupUi(this);

  m_Model = NULL;

  // The mode toolbar
  QToolBar *toolbar = new QToolBar(this);
  ui->panelToolbarMode->layout()->addWidget(toolbar);  
  toolbar->addActions(parent->GetMainToolActionGroup()->actions());

  // The action toolbar
  QToolBar *toolCmd = new QToolBar(this);
  ui->panelToolbarAction->layout()->addWidget(toolCmd);

  // Label selection button
  m_LabelSelectionButton = new LabelSelectionButton(this);

  toolCmd->addAction(FindUpstreamAction(this, "actionUndo"));
  toolCmd->addAction(FindUpstreamAction(this, "actionRedo"));
  toolCmd->addWidget(m_LabelSelectionButton);
  toolCmd->addAction(FindUpstreamAction(this, "actionLayerInspector"));

  // Add a shortcut for the button
  m_LabelSelectionButton->setShortcut(QKeySequence("l"));

  // Set up the label popup
  m_LabelSelectionPopup = new LabelSelectionPopup(this);

  // Set up the 3D toolbar
  QToolBar *tool3D = new QToolBar(this);
  ui->panelToolbarMode3D->layout()->addWidget(tool3D);
  tool3D->addActions(parent->Get3DToolActionGroup()->actions());
}

void MainControlPanel::SetModel(GlobalUIModel *model)
{
  m_Model = model;
  ui->pageCursorInspector->SetModel(m_Model->GetCursorInspectionModel());
  ui->pageZoomInspector->SetModel(m_Model);
  ui->pageLabelInspector->SetModel(m_Model);
  ui->pageDisplayInspector->SetModel(m_Model->GetDisplayLayoutModel());
  ui->pageSyncInspector->SetModel(m_Model->GetSynchronizationModel());
  ui->pagePaintbrushTool->SetModel(m_Model->GetPaintbrushSettingsModel());
  ui->pageSnakeTool->SetModel(m_Model);

  m_LabelSelectionButton->SetModel(model);
  m_LabelSelectionPopup->SetModel(model);

  // Set up state machine
  activateOnFlag(this, m_Model, UIF_BASEIMG_LOADED);

  // Listen to changes in the toolbar mode
  connectITK(m_Model->GetGlobalState()->GetToolbarModeModel(), ValueChangedEvent());
  connectITK(m_Model->GetGlobalState()->GetToolbarMode3DModel(), ValueChangedEvent());
}

void MainControlPanel::onModelUpdate(const EventBucket &bucket)
{
  // A static array of widget/mode mappings
  static QToolButton *mode_inspector_btn[] = {
    ui->btnCursorInspector,
    ui->btnZoomInspector,
    ui->btnLabelInspector,
    ui->btnToolInspector,
    ui->btnToolInspector,
    ui->btnToolInspector };

  static QWidget *mode_tool_pages[] = {
    ui->pageBlank,
    ui->pageBlank,
    ui->pageBlank,
    ui->pagePaintbrushTool,
    ui->pageSnakeTool,
    ui->pageBlank};

  // Respond to changes in toolbar mode
  GlobalState *gs = m_Model->GetGlobalState();
  if(bucket.HasEvent(ValueChangedEvent(), gs->GetToolbarModeModel()))
    {
    int mode = (int) gs->GetToolbarMode();
    mode_inspector_btn[mode]->click();
    ui->stackToolPage->setCurrentWidget(mode_tool_pages[mode]);
    }
}


MainControlPanel::~MainControlPanel()
{
  delete ui;
}

void MainControlPanel::on_btnCursorInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageCursorInspector);
    ui->grpInspector->setTitle("Cursor Inspector");
    }

}

void MainControlPanel::on_btnZoomInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageZoomInspector);
    ui->grpInspector->setTitle("Zoom Inspector");
    }
}

void MainControlPanel::on_btnLabelInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageLabelInspector);
    ui->grpInspector->setTitle("Label Inspector");
    }
}

void MainControlPanel::on_btnDisplayInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageDisplayInspector);
    ui->grpInspector->setTitle("Display Layout Inspector");
    }
}

void MainControlPanel::on_btnSyncInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageSyncInspector);
    ui->grpInspector->setTitle("Synchronization Inspector");
    }
}

void MainControlPanel::on_btnToolInspector_clicked(bool checked)
{
  if(checked)
    {
    ui->stack->setCurrentWidget(ui->pageToolInspector);
    ui->grpInspector->setTitle("Active Tool Inspector");
    }
}



