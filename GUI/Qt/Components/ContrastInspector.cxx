#include "ContrastInspector.h"
#include "ui_ContrastInspector.h"
#include "QtStyles.h"
#include "IntensityCurveModel.h"
#include "QtDoubleSpinboxCoupling.h"

ContrastInspector::ContrastInspector(QWidget *parent) :
    QWidget(parent),
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

  // Set up the couplings. This is all we have to do to make the spin box
  // play with the model! There are no callbacks to write, no event handlers
  // to worry about! Yay!!!
  new QtDoubleSpinboxCoupling(ui->inControlX, m_Model->GetMovingControlXModel());
  new QtDoubleSpinboxCoupling(ui->inControlY, m_Model->GetMovingControlYModel());
}
