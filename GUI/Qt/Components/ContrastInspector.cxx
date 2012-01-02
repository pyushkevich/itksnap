#include "ContrastInspector.h"
#include "ui_ContrastInspector.h"
#include "QtStyles.h"
#include "IntensityCurveModel.h"
#include "QtDoubleSpinboxCoupling.h"

ContrastInspector::ContrastInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::ContrastInspector)
{
  ui->setupUi(this);
  ApplyCSS(this, ":/root/itksnap.css");
}

ContrastInspector::~ContrastInspector()
{
  delete ui;
}

IntensityCurveBox * ContrastInspector::GetCurveBox()
{
  return ui->box;
}

void ContrastInspector::SetModel(IntensityCurveModel *model)
{
  // Set the model
  m_Model = model;
  ui->box->SetModel(model);

  // Listen to model update events
  connectITK(m_Model, ModelUpdateEvent());

  // Set up the couplings. This is all we have to do to make the spin box
  // play with the model! There are no callbacks to write, no event handlers
  // to worry about! Yay!!!
  new QtDoubleSpinboxCoupling(ui->inControlX, m_Model->GetMovingControlXModel());
  new QtDoubleSpinboxCoupling(ui->inControlY, m_Model->GetMovingControlYModel());

  // Set up couplings for window and level
  new QtDoubleSpinboxCoupling(ui->inLevel, m_Model->GetLevelModel());
  new QtDoubleSpinboxCoupling(ui->inWindow, m_Model->GetWindowModel());
}

void ContrastInspector::on_btnRemoveControl_clicked()
{
  m_Model->OnControlPointNumberDecreaseAction();
}

void ContrastInspector::on_btnAddControl_clicked()
{
  m_Model->OnControlPointNumberIncreaseAction();
}

void ContrastInspector::on_btnReset_clicked()
{
  m_Model->OnResetCurveAction();
}

void ContrastInspector::onModelUpdate(const EventBucket &b)
{
  m_Model->Update();
}
