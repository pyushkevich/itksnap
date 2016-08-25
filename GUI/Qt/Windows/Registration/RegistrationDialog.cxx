#include "RegistrationDialog.h"
#include "ui_RegistrationDialog.h"

#include "QtComboBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "RegistrationModel.h"
#include "QtWidgetActivator.h"
#include "QtCursorOverride.h"

Q_DECLARE_METATYPE(RegistrationModel::Transformation)
Q_DECLARE_METATYPE(RegistrationModel::SimilarityMetric)

RegistrationDialog::RegistrationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RegistrationDialog)
{
  ui->setupUi(this);
}

RegistrationDialog::~RegistrationDialog()
{
  delete ui;
}

#include "itkCommand.h"

class ProcessEventsITKCommand : public itk::Command
{
public:
  /** Standard class typedefs. */
  typedef ProcessEventsITKCommand Self;
  typedef itk::SmartPointer< Self >     Pointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(ProcessEventsITKCommand, itk::Command)

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Abstract method that defines the action to be taken by the command. */
  virtual void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    QCoreApplication::processEvents();
  }

  /** Abstract method that defines the action to be taken by the command.
   * This variant is expected to be used when requests comes from a
   * const Object */
  virtual void Execute(const itk::Object *caller, const itk::EventObject & event)
  {
    QCoreApplication::processEvents();
  }
};


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
  QtCursorOverride cursor(Qt::WaitCursor);
  m_Model->RunAutoRegistration();
}

void RegistrationDialog::on_btnLoad_clicked()
{
  // Ask for a filename
  QString selection = ShowSimpleOpenDialogWithHistory(
        this, m_Model->GetParent(), "AffineTransform",
        "Open Transform - ITK-SNAP",
        "Transform File",
        "ITK Transform Files (*.txt);; Convert3D Transform Files (*.mat)");

  // Open
  if(selection.length())
    {
    try
      {
      std::string utf = to_utf8(selection);
      m_Model->LoadTransform(utf.c_str());
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
  QString selection = ShowSimpleSaveDialogWithHistory(
        this, m_Model->GetParent(), "AffineTransform",
        "Save Transform - ITK-SNAP",
        "Transform File",
        "ITK Transform Files (*.txt);; Convert3D Transform Files (*.mat)", true);


  // Save
  if(selection.length())
    {
    try
      {
      std::string utf = to_utf8(selection);
      m_Model->SaveTransform(utf.c_str());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Transform IO Error",
                               QString("Failed to save transform file"));
      }
    }
}
