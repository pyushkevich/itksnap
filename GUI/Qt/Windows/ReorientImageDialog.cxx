#include "ReorientImageDialog.h"
#include "ui_ReorientImageDialog.h"

#include <ReorientImageModel.h>
#include <QtLineEditCoupling.h>
#include <QtLabelCoupling.h>
#include <QtComboBoxCoupling.h>
#include <QtWidgetActivator.h>

Q_DECLARE_METATYPE(ImageCoordinateGeometry::AxisDirection);

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


  // Allow the user to change the direction axis combos even when they are
  // in a state that is invalid
  QtCouplingOptions opts;
  opts.AllowUpdateInInvalidState = true;

  makeCoupling(ui->inNewAxisDirX, m_Model->GetAxisDirectionModel(0), opts);
  makeCoupling(ui->inNewAxisDirY, m_Model->GetAxisDirectionModel(1), opts);
  makeCoupling(ui->inNewAxisDirZ, m_Model->GetAxisDirectionModel(2), opts);

  // Couple status message with a model
  makeCoupling(ui->outInvalidStatus, m_Model->GetInvalidStatusModel());

  // Set up widget activation
  activateOnFlag(ui->btnApply, m_Model,
                 ReorientImageModel::UIF_VALID_NEW_RAI);
  activateOnFlag(ui->btnReverseX, m_Model,
                 ReorientImageModel::UIF_VALID_AXIS_DIRECTION_X);
  activateOnFlag(ui->btnReverseY, m_Model,
                 ReorientImageModel::UIF_VALID_AXIS_DIRECTION_Y);
  activateOnFlag(ui->btnReverseZ, m_Model,
                 ReorientImageModel::UIF_VALID_AXIS_DIRECTION_Z);

}

void ReorientImageDialog::on_btnApply_clicked()
{
  // Tell model to apply RAI code
  m_Model->ApplyCurrentRAI();
}

void ReorientImageDialog::on_btnReverseX_clicked()
{
  m_Model->ReverseAxisDirection(0);
}

void ReorientImageDialog::on_btnReverseY_clicked()
{
  m_Model->ReverseAxisDirection(1);
}

void ReorientImageDialog::on_btnReverseZ_clicked()
{
  m_Model->ReverseAxisDirection(2);
}
