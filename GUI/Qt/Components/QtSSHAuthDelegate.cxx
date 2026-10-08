#include "QtSSHAuthDelegate.h"

#include <QApplication>
#include <QCoreApplication>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QString>
#include <QThread>
#include <QVBoxLayout>
QtSSHAuthDelegate::QtSSHAuthDelegate(QWidget *parent)
  : QObject(parent)
  , m_Parent(parent)
{}

void
QtSSHAuthDelegate::RunOnMainThread(const std::function<void()> &fn)
{
  if (QThread::currentThread() == this->thread())
    fn();
  else
    QMetaObject::invokeMethod(this, fn, Qt::BlockingQueuedConnection);
}

bool
QtSSHAuthDelegate::PromptForPassword(const std::string &host,
                                     const std::string &username,
                                     const std::string &prompt,
                                     std::string       &password)
{
  bool ok = false;
  RunOnMainThread([&]
  {
    QString labelText =
      tr("Password for <b>%1@%2</b>").arg(QString::fromStdString(username), QString::fromStdString(host));

    if (!prompt.empty())
      labelText += tr("<br><small style='color:red'>%1</small>").arg(QString::fromStdString(prompt));

    QString pw = QInputDialog::getText(
      m_Parent, tr("SSH Authentication"), labelText, QLineEdit::Password, QString(), &ok);

    if (ok)
      password = pw.toStdString();
  });
  return ok;
}

bool
QtSSHAuthDelegate::PromptForPassphrase(const std::string &keyfile,
                                       const std::string &prompt,
                                       std::string       &passphrase)
{
  bool ok = false;
  RunOnMainThread([&]
  {
    QString labelText = tr("Passphrase for key <b>%1</b>").arg(QString::fromStdString(keyfile));

    if (!prompt.empty())
      labelText += tr("<br><small style='color:red'>%1</small>").arg(QString::fromStdString(prompt));

    QString pp = QInputDialog::getText(
      m_Parent, tr("SSH Authentication"), labelText, QLineEdit::Password, QString(), &ok);

    if (ok)
      passphrase = pp.toStdString();
  });
  return ok;
}

bool
QtSSHAuthDelegate::PromptForUsernameAndPassword(const std::string &host,
                                                const std::string &prompt,
                                                std::string       &username,
                                                std::string       &password)
{
  bool result = false;
  RunOnMainThread([&]
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
      return;

    username = userEdit->text().toStdString();
    password = passEdit->text().toStdString();
    result = true;
  });
  return result;
}

bool
QtSSHAuthDelegate::ConfirmHostKey(const std::string &host,
                                  const std::string &key_type,
                                  const std::string &fingerprint)
{
  bool result = false;
  RunOnMainThread([&]
  {
    QMessageBox box(m_Parent);
    box.setWindowTitle(tr("Unknown SSH Host"));
    box.setIcon(QMessageBox::Warning);
    box.setTextFormat(Qt::RichText);
    box.setText(tr("The authenticity of host <b>%1</b> cannot be established.")
                  .arg(QString::fromStdString(host).toHtmlEscaped()));
    box.setInformativeText(
      tr("The %1 key fingerprint is:<br><tt>%2</tt><br><br>"
         "If you are not sure that this is the right server, contact its administrator "
         "before connecting. Do you want to trust this host and continue?")
        .arg(QString::fromStdString(key_type).toHtmlEscaped(),
             QString::fromStdString(fingerprint).toHtmlEscaped()));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    result = (box.exec() == QMessageBox::Yes);
  });
  return result;
}

bool
QtSSHAuthDelegate::PromptForAPIKey(const std::string &server,
                                   const std::string &prompt,
                                   std::string       &api_key)
{
  bool ok = false;
  RunOnMainThread([&]
  {
    QString labelText =
      tr("API key for <b>%1</b>:").arg(QString::fromStdString(server));

    if (!prompt.empty())
      labelText += tr("<br><small style='color:red'>%1</small>").arg(
        QString::fromStdString(prompt));

    QString key = QInputDialog::getText(
      m_Parent, tr("Flywheel Authentication"), labelText,
      QLineEdit::Password, QString(), &ok);

    if (ok)
      api_key = key.toStdString();
  });
  return ok;
}
