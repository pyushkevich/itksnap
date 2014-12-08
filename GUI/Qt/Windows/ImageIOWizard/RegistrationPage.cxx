#include "ImageIOWizard/RegistrationPage.h"
#include "ui_RegistrationPage.h"
#include "SNAPQtCommon.h"
#include <QtWidgetCoupling.h>
#include <QtComboBoxCoupling.h>

Q_DECLARE_METATYPE(ImageIOWizardModel::RegistrationMode)
Q_DECLARE_METATYPE(ImageIOWizardModel::RegistrationMetric)
Q_DECLARE_METATYPE(ImageIOWizardModel::RegistrationInit)


namespace imageiowiz {

void RegistrationWorkerThread::Initialize(ImageIOWizardModel *model)
{
  // Connect the model
  m_Model = model;

  // Respond to events from the model
  AddListener<RegistrationWorkerThread>(
        m_Model, ImageIOWizardModel::RegistrationProgressEvent(),
        this, &RegistrationWorkerThread::OnProgressEvent);
}

void RegistrationWorkerThread::run()
{
  // This method executes the run registration code
  m_Model->PerformRegistration();
}

void RegistrationWorkerThread::OnProgressEvent()
{
  emit registrationProgress();
}


RegistrationPage::RegistrationPage(QWidget *parent) :
  AbstractPage(parent),
  ui(new Ui::RegistrationPage)
{
  ui->setupUi(this);
}

RegistrationPage::~RegistrationPage()
{
  delete ui;
}

void RegistrationPage::on_btnRun_clicked()
{
  RegistrationWorkerThread *workerThread = new RegistrationWorkerThread();
  workerThread->Initialize(m_Model);
  connect(workerThread, SIGNAL(registrationProgress()), this, SLOT(onRegistrationProgress()));
  connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
  workerThread->start();
}

int RegistrationPage::nextId() const
{
  return ImageIOWizard::Page_Summary;
}

void RegistrationPage::initializePage()
{
  this->setTitle("Image Registration");

  // Connect widgets to the model
  makeCoupling(ui->inRegistrationMode, this->m_Model->GetRegistrationModeModel());
  makeCoupling(ui->inSimilarityMetric, this->m_Model->GetRegistrationMetricModel());
  makeCoupling(ui->inInitialAlignment, this->m_Model->GetRegistrationInitModel());

  // Get the progress renderer and attach it to the widget
  ui->progressBox->SetRenderer(this->m_Model->GetRegistrationProgressRenderer());
}

bool RegistrationPage::validatePage()
{
  return true;
}

bool RegistrationPage::isComplete() const
{
  return true;
}

void RegistrationPage::onRegistrationProgress()
{
  this->m_Model->UpdateImageTransformFromRegistration();
  this->setTitle(QString("Metric %1").arg(m_Model->GetRegistrationObjective()));
}

}
