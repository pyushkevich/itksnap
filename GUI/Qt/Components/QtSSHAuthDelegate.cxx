#include "QtSSHAuthDelegate.h"

#include <QApplication>
#include <QCoreApplication>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>
QtSSHAuthDelegate::QtSSHAuthDelegate(QWidget *parent)
  : QObject(parent)
  , m_Parent(parent)
{}

bool
QtSSHAuthDelegate::PromptForPassword(const std::string &host,
                                     const std::string &username,
                                     const std::string &prompt,
                                     std::string       &password)
{
  QString labelText =
    tr("Password for <b>%1@%2</b>").arg(QString::fromStdString(username), QString::fromStdString(host));

  if (!prompt.empty())
    labelText += tr("<br><small style='color:red'>%1</small>").arg(QString::fromStdString(prompt));

  bool    ok = false;
  QString pw = QInputDialog::getText(
    m_Parent, tr("SSH Authentication"), labelText, QLineEdit::Password, QString(), &ok);

  if (ok)
    password = pw.toStdString();
  return ok;
}

bool
QtSSHAuthDelegate::PromptForPassphrase(const std::string &keyfile,
                                       const std::string &prompt,
                                       std::string       &passphrase)
{
  QString labelText = tr("Passphrase for key <b>%1</b>").arg(QString::fromStdString(keyfile));

  if (!prompt.empty())
    labelText += tr("<br><small style='color:red'>%1</small>").arg(QString::fromStdString(prompt));

  bool    ok = false;
  QString pp = QInputDialog::getText(
    m_Parent, tr("SSH Authentication"), labelText, QLineEdit::Password, QString(), &ok);

  if (ok)
    passphrase = pp.toStdString();
  return ok;
}

bool
QtSSHAuthDelegate::PromptForUsernameAndPassword(const std::string &host,
                                                const std::string &prompt,
                                                std::string       &username,
                                                std::string       &password)
{
  QDialog dlg(m_Parent);
  dlg.setWindowTitle(tr("SSH Authentication"));
  dlg.setWindowModality(Qt::WindowModal);

  auto *layout = new QVBoxLayout(&dlg);

  QString labelText = tr("Enter credentials for <b>%1</b>").arg(QString::fromStdString(host));
  if (!prompt.empty())
    labelText += tr("<br><small style='color:red'>%1</small>").arg(QString::fromStdString(prompt));

  auto *topLabel = new QLabel(labelText, &dlg);
  topLabel->setTextFormat(Qt::RichText);
  layout->addWidget(topLabel);

  auto *form = new QFormLayout();
  auto *userEdit = new QLineEdit(&dlg);
  auto *passEdit = new QLineEdit(&dlg);
  passEdit->setEchoMode(QLineEdit::Password);
  form->addRow(tr("Username:"), userEdit);
  form->addRow(tr("Password:"), passEdit);
  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  if (dlg.exec() != QDialog::Accepted)
    return false;

  username = userEdit->text().toStdString();
  password = passEdit->text().toStdString();
  return true;
}
