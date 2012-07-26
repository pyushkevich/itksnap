#include "SnakeToolROIPanel.h"
#include "ui_SnakeToolROIPanel.h"

#include <QtSpinBoxCoupling.h>
#include <QtWidgetArrayCoupling.h>
#include <GlobalUIModel.h>
#include <SnakeROIModel.h>

SnakeToolROIPanel::SnakeToolROIPanel(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::SnakeToolROIPanel)
{
  ui->setupUi(this);
}

SnakeToolROIPanel::~SnakeToolROIPanel()
{
  delete ui;
}

void SnakeToolROIPanel::SetModel(GlobalUIModel *model)
{
  // Store the model
  m_Model = model;

  // Hook up the couplings for the ROI size controls
  makeArrayCoupling(
        ui->inIndexX, ui->inSizeX,
        model->GetSnakeROIModel(0)->GetROIPositionAndSizeModel());
  makeArrayCoupling(
        ui->inIndexY, ui->inSizeY,
        model->GetSnakeROIModel(1)->GetROIPositionAndSizeModel());
  makeArrayCoupling(
        ui->inIndexZ, ui->inSizeZ,
        model->GetSnakeROIModel(2)->GetROIPositionAndSizeModel());
}

void SnakeToolROIPanel::on_btnResetROI_clicked()
{
  // Reset the ROI
  m_Model->GetSnakeROIModel(0)->ResetROI();
}

void SnakeToolROIPanel::on_btnAuto_clicked()
{

}
