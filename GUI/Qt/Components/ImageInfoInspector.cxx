#include "ImageInfoInspector.h"
#include "ui_ImageInfoInspector.h"

#include "ImageInfoModel.h"
#include "QtWidgetArrayCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtSpinBoxCoupling.h"

ImageInfoInspector::ImageInfoInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ImageInfoInspector)
{
  ui->setupUi(this);
}

ImageInfoInspector::~ImageInfoInspector()
{
  delete ui;
}

void ImageInfoInspector::SetModel(ImageInfoModel *model)
{
  // Store the model
  m_Model = model;

  // Create the traits objects for the various fields. This allows us to
  // specify the precision used to display these values
  FixedPrecisionRealToTextFieldWidgetTraits<double, QLineEdit> tr_real(4);

  makeArrayCoupling(ui->outDimX, ui->outDimY, ui->outDimZ,
                    m_Model->GetImageDimensionsModel());

  makeArrayCoupling(ui->outSpacingX, ui->outSpacingY, ui->outSpacingZ,
                    m_Model->GetImageSpacingModel(), tr_real);

  makeArrayCoupling(ui->outOriginX, ui->outOriginY, ui->outOriginZ,
                    m_Model->GetImageOriginModel(), tr_real);

  makeArrayCoupling(ui->outItkX, ui->outItkY, ui->outItkZ,
                    m_Model->GetImageItkCoordinatesModel(), tr_real);

  makeArrayCoupling(ui->outNiftiX, ui->outNiftiY, ui->outNiftiZ,
                    m_Model->GetImageNiftiCoordinatesModel(), tr_real);

  makeArrayCoupling(ui->inVoxX, ui->inVoxY, ui->inVoxZ,
                    m_Model->GetImageVoxelCoordinatesModel());

  makeArrayCoupling(ui->outMin, ui->outMax,
                    m_Model->GetImageMinMaxModel(), tr_real);

  makeCoupling(ui->outRAI, m_Model->GetImageOrientationModel());
}
