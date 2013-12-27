#include "SnakeToolROIPanel.h"
#include "ui_SnakeToolROIPanel.h"

#include <SNAPQtCommon.h>
#include <QtSpinBoxCoupling.h>
#include <QtWidgetArrayCoupling.h>
#include <GlobalUIModel.h>
#include <SnakeROIModel.h>
#include <MainImageWindow.h>
#include "SnakeROIResampleModel.h"
#include "GlobalState.h"

#include "ResampleDialog.h"

SnakeToolROIPanel::SnakeToolROIPanel(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::SnakeToolROIPanel)
{
  ui->setupUi(this);
  m_ResampleDialog = new ResampleDialog(this);
}

SnakeToolROIPanel::~SnakeToolROIPanel()
{
  delete ui;
}

void SnakeToolROIPanel::SetModel(GlobalUIModel *model)
{
  // Store the model
  m_Model = model;

  // Pass the model to the resample dialog
  m_ResampleDialog->SetModel(model->GetSnakeROIResampleModel());

  // Hook up the couplings for the ROI size controls
  makeArrayCoupling(
        ui->inIndexX, ui->inIndexY, ui->inIndexZ,
        model->GetSnakeROIIndexModel());

  makeArrayCoupling(
        ui->inSizeX, ui->inSizeY, ui->inSizeZ,
        model->GetSnakeROISizeModel());
}

void SnakeToolROIPanel::on_btnResetROI_clicked()
{
  // Reset the ROI
  m_Model->GetSnakeROIModel(0)->ResetROI();
}

void SnakeToolROIPanel::on_btnAuto_clicked()
{
  // TODO: Check that the label configuration is valid

  // Handle resampling if requested
  if(ui->chkResample->isChecked())
    {
    if(m_ResampleDialog->exec() != QDialog::Accepted)
      return;
    }
  else
    {
    // Make sure that no interpolation is applied
    m_Model->GetSnakeROIResampleModel()->Reset();
    m_Model->GetSnakeROIResampleModel()->Accept();
    }

  // Switch to crosshairs mode
  m_Model->GetGlobalState()->SetToolbarMode(CROSSHAIRS_MODE);

  // Show the snake panel
  MainImageWindow *main = findParentWidget<MainImageWindow>(this);
  main->OpenSnakeWizard();
}
