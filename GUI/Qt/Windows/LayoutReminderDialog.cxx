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

void LayoutReminderDialog::ConditionalExec()
{
  std::cout << "conditional exec" << std::endl;
  const GlobalDisplaySettings* dsp = this->m_GlobalUIModel->GetGlobalDisplaySettings();

  // we don't show the dialog if "never remind" flag was checked
  if (!dsp->GetFlagRemindLayoutSettings())
    return;

  // we don't show the dialog if the both settings are not checked
  if (!dsp->GetFlagLayoutPatientAnteriorShownLeft() && !dsp->GetFlagLayoutPatientRightShownLeft())
    return;

  std::string msg = "The current layout setting follows neurological convention, displaying ";

  // Only show the dialog when any of the neuralogical convention layout is set
  if (dsp->GetFlagLayoutPatientAnteriorShownLeft())
    msg += "patient's anterior on screen's left";

  if (dsp->GetFlagLayoutPatientRightShownLeft())
    {
      if (dsp->GetFlagLayoutPatientAnteriorShownLeft())
        msg += ", and ";

      msg += "patient's right on screen's left";
    }

  ui->reminderTextP1->setText(msg.c_str());

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

  std::cout << "[no clicked]" << this->m_GlobalUIModel->GetGlobalDisplaySettings()->GetFlagRemindLayoutSettings() << std::endl;
}
