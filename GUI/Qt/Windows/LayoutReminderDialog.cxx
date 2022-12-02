#include "LayoutReminderDialog.h"
#include "ui_LayoutReminderDialog.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GlobalPreferencesModel.h"
#include "PreferencesDialog.h"

LayoutReminderDialog::LayoutReminderDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LayoutReminderDialog)
{
  ui->setupUi(this);
}

LayoutReminderDialog::~LayoutReminderDialog()
{
  delete ui;
}

void LayoutReminderDialog::Initialize(GlobalUIModel *model)
{
  this->m_GlobalUIModel = model;
}

void LayoutReminderDialog::ConditionalExec(enum ExecScenarios sce)
{
  const GlobalDisplaySettings* dsp = this->m_GlobalUIModel->GetGlobalDisplaySettings();

  if (sce == ExecScenarios::Default)
    {
      // we don't show the dialog if "never remind" flag was checked
      if (!dsp->GetFlagRemindLayoutSettings())
        return;
    }
  else if (sce == ExecScenarios::Echo_Cartesian_Dicom_Loading)
    {
      // we don't show the dialog if radiological convention is selected
      if (!dsp->GetFlagLayoutPatientRightShownLeft())
        return;

      ui->chkNoRemindAgain->setDisabled(true);
    }

  QString msg_axial = "<html>On the Axial and Coronal views,&nbsp;ITK-SNAP is currently following ";


  if (dsp->GetFlagLayoutPatientRightShownLeft())
    {
      msg_axial.append("<b>radiological convention</b>");
      msg_axial.append(":&nbsp;the patient’s left side is shown on the right of the screen.&nbsp;");
    }
  else
    {
      msg_axial.append("<b>neurological convention</b>");
      msg_axial.append(":&nbsp;the patient’s left side is shown on the left of the screen.&nbsp;");
    }
  msg_axial.append("</html>");

  QString msg_sagittal = "<html>On the Sagittal view,&nbsp;";

  if (dsp->GetFlagLayoutPatientAnteriorShownLeft())
    {
      msg_sagittal.append("the patient's anterior is shown on the left of the screen.");
    }
  else
    {
      msg_sagittal.append("the patient's anterior is shown on the right of the screen.");
    }
  msg_sagittal.append("</html>");

  ui->reminderTextAxial->setText(msg_axial);
  ui->reminderTextSagittal->setText(msg_sagittal);

  this->exec();
}

void LayoutReminderDialog::SetReminderFlag()
{
  if (ui->chkNoRemindAgain->isChecked())
    {
      GlobalPreferencesModel *pm = this->m_GlobalUIModel->GetGlobalPreferencesModel();
      pm->InitializePreferences();
      pm->GetGlobalDisplaySettings()->SetFlagRemindLayoutSettings(false);
      pm->ApplyPreferences();
    }

}

void LayoutReminderDialog::on_btnYes_clicked()
{
  // before opening the preference, set the reminder-showing flag
  this->SetReminderFlag();

  // show the preference dialog
  PreferencesDialog *pref = new PreferencesDialog(this);
  pref->SetModel(this->m_GlobalUIModel->GetGlobalPreferencesModel());
  pref->ShowDialog();
  pref->GoToPage(PreferencesDialog::SliceView);

  // close reminder
  this->close();
}


void LayoutReminderDialog::on_btnNo_clicked()
{
  // before closing, set the reminder-showing flag
  this->SetReminderFlag();

  // close reminder
  this->close();
}
