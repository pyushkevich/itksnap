#include "ReorientImageDialog.h"
#include "ui_ReorientImageDialog.h"

#include <ReorientImageModel.h>
#include <OrientationGraphicRenderer.h>
#include <QtLineEditCoupling.h>
#include <QtLabelCoupling.h>
#include <QtComboBoxCoupling.h>
#include <QtTableWidgetCoupling.h>
#include <QtWidgetActivator.h>
#include <QtSimpleOpenGLBox.h>

#include <QtVTKInteractionDelegateWidget.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

Q_DECLARE_METATYPE(ImageCoordinateGeometry::AxisDirection);

ReorientImageDialog::ReorientImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ReorientImageDialog)
{
  ui->setupUi(this);

  // Create the renderer and attach to the GL box
  m_CurrentRAIRenderer = OrientationGraphicRenderer::New();
  ui->iconCurrent->SetRenderer(m_CurrentRAIRenderer);

  m_NewRAIRenderer = OrientationGraphicRenderer::New();
  ui->iconNew->SetRenderer(m_NewRAIRenderer);

  // Synchronize the cameras between the two renderers
  m_NewRAIRenderer->SyncronizeCamera(m_CurrentRAIRenderer);
}

ReorientImageDialog::~ReorientImageDialog()
{
  delete ui;
}

void ReorientImageDialog::SetModel(ReorientImageModel *model)
{
  // Set the model
  m_Model = model;

  // Pass the model to the renderer
  m_CurrentRAIRenderer->SetModel(m_Model->GetCurrentDirectionMatrixModel());
  m_NewRAIRenderer->SetModel(m_Model->GetNewDirectionMatrixModel());

  // Couple widgets to the model
  makeCoupling(ui->outCurrentRAI, m_Model->GetCurrentRAICodeModel());
  makeCoupling(ui->inNewRAI, m_Model->GetNewRAICodeModel());


  // Allow the user to change the direction axis combos even when they are
  // in a state that is invalid
  QtCouplingOptions opts(QtCouplingOptions::ALLOW_UPDATES_WHEN_INVALID);
  makeCoupling(ui->inNewAxisDirX, m_Model->GetNewAxisDirectionModel(0), opts);
  makeCoupling(ui->inNewAxisDirY, m_Model->GetNewAxisDirectionModel(1), opts);
  makeCoupling(ui->inNewAxisDirZ, m_Model->GetNewAxisDirectionModel(2), opts);

  makeCoupling(ui->outCurrentAxisDirX, m_Model->GetCurrentAxisDirectionModel(0));
  makeCoupling(ui->outCurrentAxisDirY, m_Model->GetCurrentAxisDirectionModel(1));
  makeCoupling(ui->outCurrentAxisDirZ, m_Model->GetCurrentAxisDirectionModel(2));

  makeCoupling(ui->outMatrixCurrent, m_Model->GetCurrentWorldMatrixModel());
  makeCoupling(ui->outMatrixNew, m_Model->GetNewWorldMatrixModel());

  // Couple status message with a model
  makeCoupling(ui->outInvalidStatus, m_Model->GetInvalidStatusModel());

  // Set up widget activation
  activateOnFlag(ui->inNewRAI, m_Model,
                 ReorientImageModel::UIF_IMAGE_LOADED);
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
