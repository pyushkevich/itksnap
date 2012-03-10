#include "ImageInfoInspector.h"
#include "ui_ImageInfoInspector.h"

#include "ImageInfoModel.h"
#include "QtWidgetCoupling.h"

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

  // Hook up the couplings
  makeArrayCoupling(ui->outDimX, ui->outDimY, ui->outDimZ,
                    m_Model->GetImageDimensionsModel());

  makeArrayCoupling(ui->outSpacingX, ui->outSpacingY, ui->outSpacingZ,
                    m_Model->GetImageSpacingModel());

  makeArrayCoupling(ui->outOriginX, ui->outOriginY, ui->outOriginZ,
                    m_Model->GetImageOriginModel());

  makeArrayCoupling(ui->outItkX, ui->outItkY, ui->outItkZ,
                    m_Model->GetImageItkCoordinatesModel());

  makeArrayCoupling(ui->outNiftiX, ui->outNiftiY, ui->outNiftiZ,
                    m_Model->GetImageNiftiCoordinatesModel());

  makeArrayCoupling(ui->inVoxX, ui->inVoxY, ui->inVoxZ,
                    m_Model->GetImageVoxelCoordinatesModel());
}
