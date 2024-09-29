#include "ImageInfoInspector.h"
#include "ui_ImageInfoInspector.h"

#include "ImageInfoModel.h"
#include "IntensityUnderCursorRenderer.h"
#include "QtWidgetArrayCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtReporterDelegates.h"
#include "QtPagedWidgetCoupling.h"

ImageInfoInspector::ImageInfoInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ImageInfoInspector)
{
  ui->setupUi(this);

  m_PlotBoxViewportReporter = QtViewportReporter::New();
  m_PlotBoxViewportReporter->SetClientWidget(ui->plotWidget);

  m_IntensityRenderer = IntensityUnderCursorRenderer::New();
  m_IntensityRenderer->SetBackgroundColor(Vector3d(1.0, 1.0, 1.0));
  ui->plotWidget->SetRenderer(m_IntensityRenderer);
  ui->plotWidget->setMouseTracking(true);
}

ImageInfoInspector::~ImageInfoInspector()
{
  delete ui;
}

void ImageInfoInspector::SetModel(ImageInfoModel *model)
{
  // Store the model
  m_Model = model;

  // Set the model on the renderer
  m_IntensityRenderer->SetModel(model);

  // Create the traits objects for the various fields. This allows us to
  // specify the precision used to display these values
  FixedPrecisionRealToTextFieldWidgetTraits<double, QLineEdit> tr_real(4);

  makeArrayCoupling(
        std::vector<QLineEdit *>({{ui->outDimX, ui->outDimY, ui->outDimZ, ui->outDimT}}),
        m_Model->GetImageDimensionsModel());

  makeArrayCoupling(
        std::vector<QLineEdit *>({{ui->outSpacingX, ui->outSpacingY, ui->outSpacingZ, ui->outSpacingT}}),
        m_Model->GetImageSpacingModel(), tr_real);

  makeArrayCoupling(
        std::vector<QLineEdit *>({{ui->outOriginX, ui->outOriginY, ui->outOriginZ, ui->outOriginT}}),
        m_Model->GetImageOriginModel(), tr_real);

  makeArrayCoupling(
        std::vector<QLineEdit *>({{ui->outNiftiX, ui->outNiftiY, ui->outNiftiZ, ui->outNiftiT}}),
        m_Model->GetImageNiftiCoordinatesModel(), tr_real);

  // makeArrayCoupling(ui->inVoxX, ui->inVoxY, ui->inVoxZ,
  //                  m_Model->GetReferenceSpaceVoxelCoordinatesModel());

  makeArrayCoupling(ui->outVoxelObliqueX, ui->outVoxelObliqueY, ui->outVoxelObliqueZ,
                    m_Model->GetImageVoxelCoordinatesObliqueModel(), tr_real);

  makeCoupling(ui->outVoxelObliqueT, m_Model->GetImageCurrentTimePointModel());

  makeArrayCoupling(ui->outMin, ui->outMax,
                    m_Model->GetImageMinMaxModel(), tr_real);

  makeCoupling(ui->outRAI, m_Model->GetImageOrientationModel());

  makeCoupling(ui->outPixelFormat, m_Model->GetImagePixelFormatDescriptionModel());

  // makeCoupling(ui->inVoxT, m_Model->GetImageCurrentTimePointModel());

  makeCoupling(ui->outIntensityUnderCursor, m_Model->GetImageScalarIntensityUnderCursorModel(), tr_real);

  // The page used to display the voxel coordinate depends on if the layer is in reference space
  // makePagedWidgetCoupling(ui->stkVoxelCoord, m_Model->GetImageIsInReferenceSpaceModel(),
  //                        std::map<bool, QWidget *>({{true, ui->pageReference}, {false, ui->pageOblique}}));

  // Time point can only be edited if the layer has the same number of time points as the main image
  // activateOnFlag(ui->inVoxT, m_Model, ImageInfoModel::UIF_TIME_POINT_IS_EDITABLE);

  // Turn off time dimension widgets when there is no time dimension
  activateOnFlag(QList<QObject *>({ ui->outDimT, ui->lblDimT,
                                    ui->outSpacingT, ui->lblSpacingT,
                                    ui->outOriginT, ui->lblOriginT,
                                    ui->outVoxelObliqueT, ui->lblVoxelObliqueT,
                                    ui->inVoxT, ui->lblVoxT,
                                    ui->outNiftiT, ui->lblNiftiT }),
                 m_Model, ImageInfoModel::UIF_TIME_IS_DISPLAYED, QtWidgetActivator::HideInactive);

  // How we display intensity depends on whether it is multivalued (graph) or single-valued (textbox)
  activateOnFlag(ui->grpIntensityGraph, m_Model, ImageInfoModel::UIF_INTENSITY_IS_MULTIVALUED, QtWidgetActivator::HideInactive);
  activateOnNotFlag(QList<QObject *>(
                      {ui->outIntensityUnderCursor, ui->lblIntensityUnderCursor }),
                    m_Model, ImageInfoModel::UIF_INTENSITY_IS_MULTIVALUED, QtWidgetActivator::HideInactive);
}

void ImageInfoInspector::on_btnReorientImage_clicked()
{
  FindUpstreamAction(this, "actionReorient_Image")->trigger();
}

