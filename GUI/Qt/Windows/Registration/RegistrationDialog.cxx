#include "RegistrationDialog.h"
#include "ui_RegistrationDialog.h"

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

  makeArrayCoupling(ui->inScaleXSlider, ui->inScaleYSlider, ui->inScaleZSlider,
                    m_Model->GetLogScalingModel());

  // Automatic page couplings
  makeCoupling(ui->inTransformation, m_Model->GetTransformationModel());
  makeCoupling(ui->inSimilarityMetric, m_Model->GetSimilarityMetricModel());
  makeCoupling(ui->inUseMask, m_Model->GetUseSegmentationAsMaskModel());

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
  std::vector<int> pyramid = m_Model->GetIterationPyramid();

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
  m_PlotRenderers.resize(pyramid.size());

  // Vector of box widgets
  for(int i = 0; i < pyramid.size(); i++)
    {
    if(pyramid[i] > 0)
      {
      // Create a new VTK box
      QtVTKRenderWindowBox *plot = new QtVTKRenderWindowBox(NULL);
      m_PlotRenderers[i] = OptimizationProgressRenderer::New();
      m_PlotRenderers[i]->SetModel(m_Model);
      m_PlotRenderers[i]->SetPyramidLevel(i);
      plot->SetRenderer(m_PlotRenderers[i]);
      plot->setMinimumHeight(80);

      ui->grpPlots->layout()->addWidget(plot);
      }
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
        "Open Transform - ITK-SNAP", "AffineTransform",
        "Transform File",
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
        "Save Transform - ITK-SNAP", "AffineTransform",
        "Transform File",
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
