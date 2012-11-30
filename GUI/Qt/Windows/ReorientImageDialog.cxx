#include "ReorientImageDialog.h"
#include "ui_ReorientImageDialog.h"

#include <ReorientImageModel.h>
#include <QtLineEditCoupling.h>

ReorientImageDialog::ReorientImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ReorientImageDialog)
{
  ui->setupUi(this);
}

ReorientImageDialog::~ReorientImageDialog()
{
  delete ui;
}

void ReorientImageDialog::SetModel(ReorientImageModel *model)
{
  // Set the model
  m_Model = model;

  // Couple widgets to the model
  makeCoupling(ui->outCurrentRAI, m_Model->GetCurrentRAICodeModel());
  makeCoupling(ui->inNewRAI, m_Model->GetNewRAICodeModel());
}

const char ReorientImageDialog::m_RAICodes[3][2] = {
  {'R', 'L'},
  {'A', 'P'},
  {'I', 'S'}};

const char *ReorientImageDialog::m_AxisLabels[3][2] = {
  {"Right to Left", "Left to Right"},
  {"Anterior to Posterior", "Posterior to Anterior"},
  {"Inferior to Superior", "Superior to Inferior"}};
