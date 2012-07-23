#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include <VoxelIntensityQTableModel.h>
#include <QSpinBox>
#include <GenericImageData.h>
#include <IRISApplication.h>
#include <CursorInspectionModel.h>
#include <GlobalUIModel.h>
#include <QtWidgetActivator.h>
#include <QMenu>
#include <IntensityCurveModel.h>
#include <LayerSelectionModel.h>

#include <QtSpinBoxCoupling.h>
#include <QtLineEditCoupling.h>
#include <QtWidgetArrayCoupling.h>



CursorInspector::CursorInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::CursorInspector)
{
  ui->setupUi(this);
  m_TableModel = new VoxelIntensityQTableModel(ui->tableView);

  connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(onContextMenuRequested(QPoint)));

  m_ContextMenu = new QMenu(ui->tableView);
  m_ContextMenu->addAction(ui->actionAutoContrast);
}

CursorInspector::~CursorInspector()
{
  delete ui;
}

void CursorInspector::SetModel(CursorInspectionModel *model)
{
  m_Model = model;
  m_TableModel->SetParentModel(model->GetParent());
  ui->tableView->setModel(m_TableModel);
  ui->tableView->setAlternatingRowColors(true);
  ui->tableView->setFixedWidth(160);
  ui->tableView->setFixedHeight(120);

  // Update UI from model
  UpdateUIFromModel();

  // Activators
  activateOnFlag(this, m_Model->GetParent(), UIF_BASEIMG_LOADED);

  // Couple to the model
  makeCoupling(ui->outLabelId, m_Model->GetLabelUnderTheCursorIdModel());
  makeCoupling(ui->outLabelText, m_Model->GetLabelUnderTheCursorTitleModel());

  makeArrayCoupling(ui->inCursorX, ui->inCursorY, ui->inCursorZ,
                    m_Model->GetCursorPositionModel());
}

void CursorInspector::onModelUpdate(const EventBucket &)
{
  UpdateUIFromModel();
}

void CursorInspector::UpdateUIFromModel()
{
  m_Model->Update();
}

void CursorInspector::onContextMenuRequested(QPoint pos)
{
  m_PopupRow = ui->tableView->rowAt(pos.y());
  if(m_PopupRow >= 0)
    m_ContextMenu->popup(QCursor::pos());
}

void CursorInspector::on_actionAutoContrast_triggered()
{
  LayerIterator it =
      m_Model->GetParent()->GetLoadedLayersSelectionModel()->GetNthLayer(m_PopupRow);

  if(it.GetLayerAsGray())
    {
    // Select the currently highlighted layer
    m_Model->GetParent()->GetIntensityCurveModel()->SetLayer(it.GetLayerAsGray());

    // Auto-adjust intensity in the selected layer
    m_Model->GetParent()->GetIntensityCurveModel()->OnAutoFitWindow();
    }
}
