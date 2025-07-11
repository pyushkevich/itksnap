#include <QtGui/qdesktopservices.h>
#include <QtWidgets/qfiledialog.h>

#include "DeepLearningServerEditor.h"
#include "QProcessOutputTextWidget.h"
#include "QtPagedWidgetCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "ui_DeepLearningServerEditor.h"

#include "QtComboBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtWidgetActivator.h"

#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QCoreApplication>
#include <QSet>
#include <QtCore/qthread.h>

void
PythonFinderWorker::findPythonInterpreters()
{
#ifdef Q_OS_WIN
  const QChar sep = ';';
  QStringList pythonNames = { "python.exe", "python3.exe", "pythonw.exe" };
#else
  const QChar sep = ':';
  QStringList pythonNames = { "python", "python3" };
#endif

  QStringList   interpreters;
  QSet<QString> seen; // To avoid duplicates

  // Get PATH environment variable
  QString     pathEnv = qgetenv("PATH");
  QStringList paths = pathEnv.split(sep, Qt::SkipEmptyParts);

  // Search each directory in PATH
  for (const QString &dirPath : paths)
  {
    QDir dir(dirPath);
    for (const QString &pyName : pythonNames)
    {
      QString   fullPath = dir.filePath(pyName);
      QFileInfo fi(fullPath);
      if (fi.exists() && fi.isExecutable() && !seen.contains(fi.canonicalFilePath()))
      {
        interpreters.append(fi.canonicalFilePath());
        seen.insert(fi.canonicalFilePath());
      }
    }
  }

  // Optionally, add standard locations (macOS/Linux)
#ifndef Q_OS_WIN
  QStringList extraDirs = { "/usr/local/bin", "/usr/bin", QDir::homePath() + "/.local/bin" };
  for (const QString &dirPath : extraDirs)
  {
    QDir dir(dirPath);
    for (const QString &pyName : pythonNames)
    {
      QString   fullPath = dir.filePath(pyName);
      QFileInfo fi(fullPath);
      if (fi.exists() && fi.isExecutable() && !seen.contains(fi.canonicalFilePath()))
      {
        interpreters.append(fi.canonicalFilePath());
        seen.insert(fi.canonicalFilePath());
      }
    }
  }
#endif

  emit interpretersFound(interpreters);
}

std::pair<bool, QString> checkVEnvFolderStatus(const QString &path)
{
  // Check if the path exists
  QDir dir(path);
  if (!dir.exists())
    return std::make_pair(false, "Package directory has not been created");

  // Check if the Venv has been created
  QFile cfgFile(QDir(path).filePath("pyvenv.cfg"));
  if (!cfgFile.exists())
  {
    return std::make_pair(false, "Package directory has not been initialized");
  }

  if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qWarning() << "Unable to open pyvenv.cfg in" << path;
    return std::make_pair(false, "Package directory does not contain a valid virtual environment");
  }

  // Get the Python version
  QString version;
  QTextStream in(&cfgFile);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.startsWith("version")) {
      auto parts = line.split('=', Qt::SkipEmptyParts);
      if (parts.size() == 2) {
        version = parts[1].trimmed();
      }
    }
  }

  if(!version.size())
    return std::make_pair(false, "Package directory does not contain a valid virtual environment");

  return std::make_pair(true, QString("Package directory contains a valid Python %1 virtual environment").arg(version));

}


PythonProcess::PythonProcess(const QString     &pythonExe,
                             const QStringList &args,
                             QProcessOutputTextWidget    *outputWidget,
                             QObject           *parent)
  : QObject(parent)
  , m_PythonExe(pythonExe)
  , m_Args(args)
  , m_OutputWidget(outputWidget)
{
  // Set environment for the process
  m_Process = new QProcess(this);

  connect(m_Process, &QProcess::readyReadStandardOutput, outputWidget, &QProcessOutputTextWidget::appendStdout);
  connect(m_Process, &QProcess::readyReadStandardError, outputWidget, &QProcessOutputTextWidget::appendStderr);
  connect(m_Process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &PythonProcess::onFinished);
  connect(m_Process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &PythonProcess::finished);
}

void PythonProcess::start()
{
  m_OutputWidget->appendPlainText("---------------------------------------\n");
  m_OutputWidget->appendPlainText(QString("Running %1 %2\n").arg(m_PythonExe, m_Args.join(" ")));
  m_OutputWidget->appendPlainText("---------------------------------------\n");
  m_Process->start(m_PythonExe, m_Args);
}

bool PythonProcess::waitForFinished()
{
  return m_Process->waitForFinished();
}

void PythonProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
  m_OutputWidget->appendPlainText(QString("Process %1 finished with code %2\n").arg(m_PythonExe).arg(exitCode));
  this->deleteLater();
}


DeepLearningServerEditor::DeepLearningServerEditor(QWidget *parent)
  : SNAPComponent(parent)
  , ui(new Ui::DeepLearningServerEditor)
{
  ui->setupUi(this);

  // Hide the text edit
  ui->txtInstallLog->hide();
}

DeepLearningServerEditor::~DeepLearningServerEditor() { delete ui; }



void DeepLearningServerEditor::StartPythonExeSearch()
{
  // Build the list of known Python environments
  auto *worker = new PythonFinderWorker();
  auto *thread = new QThread();
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &PythonFinderWorker::findPythonInterpreters);
  connect(worker, &PythonFinderWorker::interpretersFound, this, [this, thread, worker](const QStringList &interpreters) {
    for(auto &exe : interpreters)
    {
      m_Model->AddKnownLocalPythonExePath(exe.toStdString());
    }
    thread->quit();
    thread->wait();
    worker->deleteLater();
    thread->deleteLater();
  });

  thread->start();
}

class DeepLearningServeModeDescriptionTraits
{
public:
  using Value = bool;
  static QString GetText(int row, const Value &m)
  {
    return m ? "Remote Connection" : "Local Python";
  }

  static QIcon GetIcon(int, const Value &)
  {
    return QIcon();
  }

  static QVariant GetIconSignature(int, const Value &)
  {
    return QVariant(0);
  }
};

void
DeepLearningServerEditor::SetModel(DeepLearningServerPropertiesModel *model)
{
  m_Model = model;
  makeRadioGroupCoupling(ui->radioConnRemote, ui->radioConnLocal, m_Model->GetRemoteConnectionModel());
  makeCoupling(ui->inNickname, m_Model->GetNicknameModel());
  makeCoupling(ui->inHostname, m_Model->GetHostnameModel());
  makeCoupling(ui->inPort, m_Model->GetPortModel());
  makeCoupling(ui->chkTunnel, m_Model->GetUseSSHTunnelModel());
  makeCoupling(ui->inSSHUsername, m_Model->GetSSHUsernameModel());
  makeCoupling(ui->inSSHPrivateKey, m_Model->GetSSHPrivateKeyFileModel());
  makeCoupling(ui->outURL, m_Model->GetFullURLModel());
  makeCoupling(ui->inPythonVEnv, m_Model->GetLocalPythonVEnvPathModel());
  makeCoupling(ui->inPythonExe, m_Model->GetLocalPythonExePathModel());
  makeCoupling(ui->chkNoSSLVerify, m_Model->GetNoSSLVerifyModel());

  makeWidgetVisibilityCoupling(ui->grpSSH, m_Model->GetUseSSHTunnelModel(), false);
  makePagedWidgetCoupling(ui->stackMode,
                          m_Model->GetRemoteConnectionModel(),
                          std::map<bool, QWidget *>({ { true, ui->pageRemote }, { false , ui->pageLocal }}));

  // Listen to events on the model
  this->connectITK(m_Model->GetLocalPythonVEnvPathModel(), ValueChangedEvent());
  this->connectITK(m_Model->GetRemoteConnectionModel(), ValueChangedEvent());

  // If the virtual environment path is empty in the model, set it to the default
  if(!m_Model->GetRemoteConnection() && m_Model->GetLocalPythonVEnvPath().empty())
  {
    // TODO: in the future only do this if there is not already an entry pointing to this Venv
    QString venvPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/dls_venv";
    m_Model->SetLocalPythonVEnvPath(venvPath.toStdString());
  }

  // Search for local python exes in the background
  StartPythonExeSearch();

  // Listen for changes in the python virtual environment path
}

void
DeepLearningServerEditor::onModelUpdate(const EventBucket &bucket)
{
  bool venv_changed = bucket.HasEvent(ValueChangedEvent(), m_Model->GetLocalPythonVEnvPathModel());
  bool remote_mode_changed = bucket.HasEvent(ValueChangedEvent(), m_Model->GetRemoteConnectionModel());

  // Handle change in the virtual environment path
  if(venv_changed || remote_mode_changed)
  {
    // If not in remote mode, we need to check the path for whether it contains a valid virtual environment
    if(!m_Model->GetRemoteConnection())
    {
      updateVEnvStatusDisplay();
    }
  }
}


void
DeepLearningServerEditor::on_btnConfigurePackages_clicked()
{
  // Install the environment with pip
  ui->txtInstallLog->show();
  ui->txtInstallLog->clear();

  // Get the python exe to run
  auto python_exe = QString::fromStdString(m_Model->GetLocalPythonExePath());

  // Use the current python to install the virtual environment
  QStringList args;
  args << "-m" << "venv" << "--clear" << ui->inPythonVEnv->text();
  auto *venv = new PythonProcess(python_exe, args, ui->txtInstallLog, this);
  connect(venv, &PythonProcess::finished, this, &DeepLearningServerEditor::on_VEnvInstallFinished);
  venv->start();
}

void
DeepLearningServerEditor::updateVEnvStatusDisplay()
{
  QString venv_path = QString::fromStdString(m_Model->GetLocalPythonVEnvPath());
  QString venv_status_str;
  bool venv_status;
  std::tie(venv_status, venv_status_str) = checkVEnvFolderStatus(venv_path);
  ui->lblPythonVEnvStatus->setText(venv_status_str);
}

void
DeepLearningServerEditor::on_VEnvInstallFinished(int exitCode, QProcess::ExitStatus status)
{
  updateVEnvStatusDisplay();
  if(exitCode == 0 && status == QProcess::NormalExit)
  {
    // Find the python
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("Scripts/python.exe");
#else
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("bin/python");
#endif

    // Upgrade pip
    QStringList args;
    args << "-m" << "pip" << "install" << "--upgrade" << "pip";
    auto *pip = new PythonProcess(venvPython, args, ui->txtInstallLog, this);
    connect(pip, &PythonProcess::finished, this, &DeepLearningServerEditor::on_PipUpgradePipFinished);
    pip->start();
  }
}

void
DeepLearningServerEditor::on_PipUpgradePipFinished(int exitCode, QProcess::ExitStatus status)
{
  if(exitCode == 0 && status == QProcess::NormalExit)
  {
    // Find the python
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("Scripts/python.exe");
#else
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("bin/python");
#endif

    // TODO: the desired version of itksnap-dls should not go here!
    // This is where you would do the pip install
    QStringList args;
    args << "-m" << "pip" << "install" << "itksnap-dls>=0.0.6";
    auto *pip = new PythonProcess(venvPython, args, ui->txtInstallLog, this);
    connect(pip, &PythonProcess::finished, this, &DeepLearningServerEditor::on_PipInstallDLSFinished);
    pip->start();
  }
}

void
DeepLearningServerEditor::on_PipInstallDLSFinished(int exitCode, QProcess::ExitStatus status)
{
  updateVEnvStatusDisplay();
  if(exitCode == 0 && status == QProcess::NormalExit)
  {
    // Find the python
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("Scripts/python.exe");
#else
    venvPython = QDir(ui->inPythonVEnv->text()).filePath("bin/python");
#endif

    // This is where you would do the pip install
    // TODO: need special mode to test the server without downloading
    QStringList args;
    args << "-m" << "itksnap_dls" << "--setup-only";
    auto *pip = new PythonProcess(venvPython, args, ui->txtInstallLog, this);
    connect(pip, &PythonProcess::finished, this, &DeepLearningServerEditor::on_SetupDLSFinished);
    pip->start();
  }
}

void
DeepLearningServerEditor::on_SetupDLSFinished(int exitCode, QProcess::ExitStatus status)
{
  if(exitCode == 0 && status == QProcess::NormalExit)
  {
    ui->txtInstallLog->append("<b>=======================================</b>\n");
    ui->txtInstallLog->append("<b>ITK-SNAP Deep Learning Server is Ready!</b>\n");
    ui->txtInstallLog->append("<b>=======================================</b>\n");
  }
}



void
DeepLearningServerEditor::on_btnResetVEnvFolderToDefault_clicked()
{
  auto default_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/dls_venv";
  m_Model->SetLocalPythonVEnvPath(default_path.toStdString());
}

void
DeepLearningServerEditor::on_btnFindPythonExe_clicked()
{
#ifdef Q_OS_WIN
  QString defaultName = "python.exe python3.exe pythonw.exe";
  QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "\\AppData\\Local\\Programs\\Python";
#else
  QString defaultName = "python python3 python3.*";
  QString defaultDir = "/usr/bin";
#endif

  QString filter = QString("Python Interpreter (%1)").arg(defaultName);
  QString file = QFileDialog::getOpenFileName(this, tr("Select Python Interpreter"), defaultDir, filter);
  if(!file.isNull())
    m_Model->SetLocalPythonExePath(file.toStdString());
}

void
DeepLearningServerEditor::on_btnFindVEnvFolder_clicked()
{
  QString dir =
    QFileDialog::getExistingDirectory(this,
                                      "Select Python Virtual Environment Directory",
                                      QString::fromStdString(m_Model->GetLocalPythonExePath()));

  if(!dir.isNull())
  {
    m_Model->SetLocalPythonVEnvPath(dir.toStdString());
  }
}
