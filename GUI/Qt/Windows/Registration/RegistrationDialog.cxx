#include "RegistrationDialog.h"
#include "ui_RegistrationDialog.h"

#include <QMenu>
#include <QVBoxLayout>
#include "QtComboBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "RegistrationModel.h"
#include "QtWidgetActivator.h"
#include "QtCursorOverride.h"
#include "SimpleFileDialogWithHistory.h"
#include "ProcessEventsITKCommand.h"
#include "OptimizationProgressRenderer.h"

#include "QtVTKRenderWindowBox.h"

Q_DECLARE_METATYPE(RegistrationModel::Transformation)
Q_DECLARE_METATYPE(RegistrationModel::SimilarityMetric)

RegistrationDialog::RegistrationDialog(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::RegistrationDialog)
{
  ui->setupUi(this);  

  // Set up a menu
  QMenu *menuMatch = new QMenu(this);
  menuMatch->addAction(ui->actionImage_Centers);
  menuMatch->addAction(ui->actionCenters_of_Mass);
  menuMatch->addAction(ui->actionMoments_of_Inertia);
  ui->btnMatchCenters->setMenu(menuMatch);
}

RegistrationDialog::~RegistrationDialog()
{
  delete ui;
}



void RegistrationDialog::SetModel(RegistrationModel *model)
{
  m_Model = model;

  makeCoupling(ui->inMovingLayer, m_Model->GetMovingLayerModel());

  // Manual page couplings
  makeCoupling((QAbstractButton *) ui->btnInteractiveTool, m_Model->GetInteractiveToolModel());

  makeArrayCoupling(ui->inRotX, ui->inRotY, ui->inRotZ,
                    m_Model->GetEulerAnglesModel());

  makeArrayCoupling(ui->inRotXSlider, ui->inRotYSlider, ui->inRotZSlider,
                    m_Model->GetEulerAnglesModel());

  makeArrayCoupling(ui->inTranX, ui->inTranY, ui->inTranZ,
                    m_Model->GetTranslationModel());

  makeArrayCoupling(ui->inTranXSlider, ui->inTranYSlider, ui->inTranZSlider,
                    m_Model->GetTranslationModel());

  makeArrayCoupling(ui->inScaleX, ui->inScaleY, ui->inScaleZ,
                    m_Model->GetScalingModel());

  makeArrayCoupling((QAbstractButton *) ui->btnFlipX, (QAbstractButton *) ui->btnFlipY, (QAbstractButton *) ui->btnFlipZ,
                    m_Model->GetFlipModel());

  makeArrayCoupling(ui->inScaleXSlider, ui->inScaleYSlider, ui->inScaleZSlider,
                    m_Model->GetLogScalingModel());

  // Automatic page couplings
  makeCoupling(ui->inTransformation, m_Model->GetTransformationModel());
  makeCoupling(ui->inSimilarityMetric, m_Model->GetSimilarityMetricModel());
  makeCoupling(ui->inUseMask, m_Model->GetUseSegmentationAsMaskModel());

  makeCoupling(ui->inCoarseLevel, m_Model->GetCoarsestResolutionLevelModel());
  makeCoupling(ui->inFineLevel, m_Model->GetFinestResolutionLevelModel());

  activateOnFlag(ui->inMovingLayer, m_Model,
                 RegistrationModel::UIF_MOVING_SELECTION_AVAILABLE);
  activateOnFlag(ui->pgManual, m_Model,
                 RegistrationModel::UIF_MOVING_SELECTED);

  // This command just updates the GUI after each iteration - causing the images to
  // jitter over different iterations
  ProcessEventsITKCommand::Pointer cmdProcEvents = ProcessEventsITKCommand::New();
  m_Model->SetIterationCommand(cmdProcEvents);

}

void RegistrationDialog::on_pushButton_clicked()
{
  m_Model->SetCenterOfRotationToCursor();
}

void RegistrationDialog::on_pushButton_2_clicked()
{
  m_Model->ResetTransformToIdentity();
}

void RegistrationDialog::on_btnRunRegistration_clicked()
{
  // Create the render panels based on the number of iterations
  int coarsest = m_Model->GetCoarsestResolutionLevel();
  int finest = m_Model->GetFinestResolutionLevel();
  int n_levels = 1 + (coarsest - finest);
  assert(n_levels > 0);

  // Delete all existing render boxes
  QList<QtVTKRenderWindowBox *> bx = ui->grpPlots->findChildren<QtVTKRenderWindowBox*>();
  foreach (QtVTKRenderWindowBox * w, bx)
    delete w;

  // Turn on the wait cursor
  QtCursorOverride cursor(Qt::WaitCursor);

  // Update the user interface
  QCoreApplication::processEvents();

  // Vector of renderers - so they don't disappear
  m_PlotRenderers.clear();
  m_PlotRenderers.resize(n_levels);

  // Background color
  QPalette pMain = ui->pgAuto->palette(), pScroll = ui->scrollPlots->palette();
  pScroll.setBrush(QPalette::Window, pMain.background());
  ui->scrollPlots->setPalette(pScroll);
  ui->scrollAreaWidgetContents->setPalette(pScroll);

  // Vector of box widgets
  for(int k = 0; k < n_levels; k++)
    {
    // Create a new VTK box
    QtVTKRenderWindowBox *plot = new QtVTKRenderWindowBox(NULL);
    m_PlotRenderers[k] = OptimizationProgressRenderer::New();
    m_PlotRenderers[k]->SetModel(m_Model);
    m_PlotRenderers[k]->SetPyramidLevel(k);
    m_PlotRenderers[k]->SetPyramidZoom(1 << (coarsest - k));
    plot->SetRenderer(m_PlotRenderers[k]);
    plot->setMinimumHeight(76);
    plot->setMaximumHeight(76);

    ui->grpPlots->layout()->addWidget(plot);
    }

  m_Model->RunAutoRegistration();
}

int RegistrationDialog::GetTransformFormat(QString &format)
{
  if(format == "ITK Transform Files")
    return RegistrationModel::FORMAT_ITK;
  else if(format == "Convert3D Transform Files")
    return RegistrationModel::FORMAT_C3D;
  else
    return RegistrationModel::FORMAT_ITK;
}

void RegistrationDialog::on_btnLoad_clicked()
{
  // Ask for a filename
  SimpleFileDialogWithHistory::QueryResult result =
      SimpleFileDialogWithHistory::showOpenDialog(
        this, m_Model->GetParent(),
        "Open Transform - ITK-SNAP", "Transform File",
        "AffineTransform",
        "ITK Transform Files (*.txt);; Convert3D Transform Files (*.mat)");

  RegistrationModel::TransformFormat format =
      (RegistrationModel::TransformFormat) this->GetTransformFormat(result.activeFormat);


  // Open
  if(result.filename.length())
    {
    try
      {
      std::string utf = to_utf8(result.filename);
      m_Model->LoadTransform(utf.c_str(), format);
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Transform IO Error",
                               QString("Failed to load transform file"));
      }
    }
}

void RegistrationDialog::on_btnSave_clicked()
{
  // Ask for a filename
  SimpleFileDialogWithHistory::QueryResult result =
      SimpleFileDialogWithHistory::showSaveDialog(
        this, m_Model->GetParent(),
        "Save Transform - ITK-SNAP", "Transform File",
        "AffineTransform",
        "ITK Transform Files (*.txt);; Convert3D Transform Files (*.mat)", true);

  RegistrationModel::TransformFormat format =
      (RegistrationModel::TransformFormat) this->GetTransformFormat(result.activeFormat);

  // Save
  if(result.filename.length())
    {
    try
      {
      std::string utf = to_utf8(result.filename);
      m_Model->SaveTransform(utf.c_str(), format);
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Transform IO Error",
                               QString("Failed to save transform file"));
      }
    }
}

void RegistrationDialog::on_buttonBox_clicked(QAbstractButton *button)
{
  // Tell the model the dialog is closing
  m_Model->OnDialogClosed();

  // The only button is close
  emit wizardFinished();
}

void RegistrationDialog::on_tabWidget_currentChanged(int index)
{
  // Activate the interactive tool when the user switches to the manual page
  if(ui->tabWidget->currentWidget() == ui->pgManual)
    m_Model->GetInteractiveToolModel()->SetValue(true);
}

#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>

void RegistrationDialog::on_btnReslice_clicked()
{
  // Prompt the user for the kind of reslicing to do
  QDialog *dialog = new QDialog(this);
  QFormLayout *lo = new QFormLayout();

  dialog->setWindowTitle("Reslicing Options - ITK-SNAP");

  // Set up interpolation options
  QComboBox *cbInterp = new QComboBox(dialog);
  cbInterp->addItem("Nearest Neighbor", QVariant(NEAREST_NEIGHBOR));
  cbInterp->addItem("Linear", QVariant(TRILINEAR));
  cbInterp->setCurrentIndex(1);
  lo->addRow("&Interpolation:", cbInterp);

  // Set up background value (currently unimplemented)
  /*
  QDoubleSpinBox *inBkValue = new QDoubleSpinBox(dialog);
  inBkValue->setValue(0.0);
  inBkValue->setToolTip(
        "Intensity value that will be assigned to voxels that are "
        "outside of the image domain.");
  lo->addRow("&Background intensity:", inBkValue);
  */

  QLabel *label = new QLabel;
  label->setText("The resliced image will be created as an addtional\n"
                 "image layer. You can save the resliced image using\n"
                 "the context menu.");
  lo->addRow(label);

  QDialogButtonBox *bbox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(bbox, SIGNAL(accepted()), dialog, SLOT(accept()));
  connect(bbox, SIGNAL(rejected()), dialog, SLOT(reject()));
  lo->addRow(bbox);

  dialog->setLayout(lo);


  // Get the selected values
  if(dialog->exec() == QDialog::Accepted)
    {
    bool is_linear = (cbInterp->currentText() == "Linear");
    m_Model->ResliceMovingImage(is_linear ? TRILINEAR : NEAREST_NEIGHBOR);
    }

  delete dialog;
}

void RegistrationDialog::on_actionImage_Centers_triggered()
{
  m_Model->MatchImageCenters();
}

void RegistrationDialog::on_actionCenters_of_Mass_triggered()
{
  // Turn on the wait cursor
  QtCursorOverride cursor(Qt::WaitCursor);
  m_Model->MatchByMoments(1);
}

void RegistrationDialog::on_actionMoments_of_Inertia_triggered()
{
  // Turn on the wait cursor
  QtCursorOverride cursor(Qt::WaitCursor);
  m_Model->MatchByMoments(2);
}
