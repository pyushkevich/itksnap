#include "MainControlPanel.h"
#include "ui_MainControlPanel.h"
#include "GlobalUIModel.h"
#include "QtWidgetActivator.h"


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

MainControlPanel::MainControlPanel(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::MainControlPanel)
{
  ui->setupUi(this);

  m_Model = NULL;

  QToolBar *toolbar = new QToolBar(this);
  ui->grpToolbox->layout()->addWidget(toolbar);

  QActionGroup *ag = new QActionGroup(this);
  ag->addAction(ui->actionCrosshair);
  ag->addAction(ui->actionZoomPan);
  ag->addAction(ui->actionPolygon);
  ag->addAction(ui->actionPaintbrush);
  ag->addAction(ui->actionSnake);

  toolbar->addActions(ag->actions());
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

  // Set up state machine
  activateOnFlag(this, m_Model, UIF_BASEIMG_LOADED);
  activateOnNotFlag(ui->actionPolygon, m_Model, UIF_SNAKE_MODE);
  activateOnNotFlag(ui->actionPaintbrush, m_Model, UIF_SNAKE_MODE);
  activateOnNotFlag(ui->actionSnake, m_Model, UIF_SNAKE_MODE);

  // Listen to changes in the toolbar mode
  connectITK(m_Model->GetToolbarModeModel(), ValueChangedEvent());
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
  if(bucket.HasEvent(ValueChangedEvent()))
    {
    int mode = (int) m_Model->GetToolbarMode();
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


void MainControlPanel::on_actionCrosshair_triggered(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(CROSSHAIRS_MODE);
    }
}

void MainControlPanel::on_actionZoomPan_triggered(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(NAVIGATION_MODE);
    }
}

void MainControlPanel::on_actionPolygon_triggered(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(POLYGON_DRAWING_MODE);
    }
}

void MainControlPanel::on_actionPaintbrush_triggered(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(PAINTBRUSH_MODE);
    }
}

void MainControlPanel::on_actionSnake_triggered(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(ROI_MODE);
    }
}
