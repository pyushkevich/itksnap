#include "MeshImportFileSelectionPage.h"
#include "ui_MeshImportFileSelectionPage.h"
#include "MeshImportModel.h"
#include "MeshImportWizard.h"
#include "SNAPQtCommon.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "qfileinfo.h"
#include <QMessageBox>


MeshImportFileSelectionPage::MeshImportFileSelectionPage(QWidget *parent) :
  QWizardPage(parent),
  ui(new Ui::MeshImportFileSelectionPage)
{
  ui->setupUi(this);

  this->registerField(
        "Filename*", ui->filePanel,
        "absoluteFilename", SIGNAL(absoluteFilenameChanged(QString)));
  this->registerField(
        "Format*", ui->filePanel,
        "activeFormat", SIGNAL(activeFormatChanged(QString)));
}

MeshImportFileSelectionPage::~MeshImportFileSelectionPage()
{
  delete ui;
}

void MeshImportFileSelectionPage::SetModel(MeshImportModel *model)
{
  this->m_Model = model;
}

void MeshImportFileSelectionPage::initializePage()
{
  assert(m_Model);

  // Set title based on current mode
  setTitle(m_Model->GetTitle().c_str());

  // Set filter for the file panel
  QString filter;

  // Get the domain of importable format
  MeshImportModel::FileFormat value;
  MeshImportModel::FileFormatDomain domain;
  m_Model->GetImportFormatModel()->GetValueAndDomain(value, &domain);

  for (auto cit = domain.begin(); cit != domain.end(); ++cit)
    {
      if (cit != domain.begin())
        filter.append(";; ");

      GuidedMeshIO::MeshFormatDescriptor mfd = GuidedMeshIO::m_MeshFormatDescriptorMap.at(cit->first);
      std::ostringstream oss;

      // parse extension list into the filter, separated by space
      for (auto extIt = mfd.extensions.cbegin(); extIt != mfd.extensions.cend(); ++extIt)
        {
          if (extIt != mfd.extensions.cbegin())
            oss << ' ';

          oss << *extIt;
        }

      filter.append(QString("%1 (%2)")
                    .arg(from_utf8(mfd.name))
                    .arg(from_utf8(oss.str())));
    }

  auto mode = m_Model->GetMode();
  if (mode == MeshImportModel::SINGLE)
    {
    // Create the file panel
    ui->filePanel->initializeForOpenFile(
          m_Model->GetParentModel(),
          "Mesh file name: ",
          from_utf8(m_Model->GetHistoryName()),
          filter,
          QString(), from_utf8(domain[GuidedMeshIO::FORMAT_VTK]));
    }
  else
    {
    // Create directory file panel
    ui->filePanel->initializeForOpenFiles(
          m_Model->GetParentModel(),
          "Mesh series files: ",
          from_utf8(m_Model->GetHistoryName()),
          filter,
          QString(), from_utf8(domain[GuidedMeshIO::FORMAT_VTK]));
    }
}

QString GetErrorText(std::string input)
{
  return QString("<span style=\" color:#7f0000;\">%1</span>").arg(input.c_str());
}


bool MeshImportFileSelectionPage::validatePage()
{
  // Clear error state
  ui->lblMessage->clear();

  // Get selected format from the file panel
  QString format = ui->filePanel->activeFormat();
  QStringList filenames = ui->filePanel->GetSelectedFiles();

  // User manually put filename instead of using the browser
  if (filenames.size() == 0)
    {
    auto fn = ui->filePanel->absoluteFilename();
    QFileInfo info(fn);
    if (info.exists() && info.isFile())
      filenames.push_back(ui->filePanel->absoluteFilename());
    else
      {
      ui->lblMessage->setText(GetErrorText("File does not exist!"));
      return false;
      }
    }


  auto nt = m_Model->GetParentModel()->GetDriver()->GetNumberOfTimePoints();
  if (nt < filenames.length())
    {
    std::ostringstream oss;
    oss << "Number of selected files (" << filenames.length()
        << ") cannot exceed the number of time points (" << nt << ")!";
    ui->lblMessage->setText(GetErrorText(oss.str()));
    ui->lblMessage->setWordWrap(true);
    return false;
    }

  // Start loading
  std::vector<std::string> fn_list;
  for (auto fn : filenames)
    fn_list.push_back(fn.toStdString());

  std::sort(fn_list.begin(), fn_list.end());
  const unsigned int crntTP = m_Model->GetParentModel()->GetDriver()->GetCursorTimePoint();
  const unsigned int displayTP =  crntTP + 1;
  MeshImportModel::FileFormat fmt = m_Model->GetFileFormatByName(to_utf8(format));

  // First we handle two easy cases
  if (nt == 1)
    {
    // For 3D workspace, direct load to new layer
    m_Model->Load(fn_list, fmt);
    return true;
    }

  if (fn_list.size() == 1 && !m_Model->IsActiveMeshStandalone())
    {
    // Must create new layer. Only remind user about current TP
    QMessageBox *boxNewLayer = MeshImportWizard::CreateLoadToNewLayerMessageBox(this, displayTP);
    boxNewLayer->setObjectName("msgboxNewLayer");
    int ret = boxNewLayer->exec();
    delete boxNewLayer;

    switch (ret)
      {
      case QMessageBox::Ok:
        {
        m_Model->Load(fn_list, fmt);
        return true;
        }
      case QMessageBox::Cancel:
      default:
        {
        return false;
        }
      }
    }

  // For more complicated cases, use a message box
  // -- Active Mesh Layer is a standalone mesh
  QMessageBox *msgBox = new QMessageBox(this);
  QPushButton *btnSeriesFromTP = nullptr;
  QPushButton *btnSeriesFromBegin = nullptr;
  QPushButton *btnNewLayer = nullptr;
  QPushButton *btnLoadTP = nullptr;

  if (fn_list.size() > 1)
    {
    msgBox->setText("How do you want the mesh series to be loaded?");
    std::ostringstream oss;
    oss << "From Current Time Point (" << displayTP << ")";
    btnSeriesFromTP = msgBox->addButton(tr(oss.str().c_str()), QMessageBox::ActionRole);
    btnSeriesFromBegin = msgBox->addButton(tr("From the Beginning"), QMessageBox::ActionRole);
    }
  else
    {
    msgBox->setText("How do you want the mesh to be loaded?");
    std::ostringstream oss;
    oss << "To Current Time Point (" << displayTP << ")";
    btnLoadTP = msgBox->addButton(tr(oss.str().c_str()), QMessageBox::ActionRole);
    btnNewLayer = msgBox->addButton(tr("To a New Layer"), QMessageBox::ActionRole);
    }

  msgBox->setStandardButtons(QMessageBox::Cancel);
  msgBox->setDefaultButton(btnNewLayer);
  int ret = msgBox->exec();
  auto clicked = msgBox->clickedButton();
  delete msgBox;

  if (ret == QMessageBox::Cancel)
    {
    return false;
    }

  if (clicked == (QAbstractButton*)btnNewLayer)
    {
    QMessageBox *boxNewLayer = MeshImportWizard::CreateLoadToNewLayerMessageBox(this, displayTP);
    int ret = boxNewLayer->exec();
    delete boxNewLayer;

    switch (ret)
      {
      case QMessageBox::Ok:
        {
        m_Model->Load(fn_list, fmt, displayTP);
        return true;
        }
      case QMessageBox::Cancel:
      default:
        {
        return false;
        }
      }
    }
  else if (clicked == (QAbstractButton*)btnLoadTP)
    {
    QMessageBox *boxLoadTP = MeshImportWizard::CreateLoadToTimePointMessageBox(this, displayTP);
    int ret = boxLoadTP->exec();
    delete boxLoadTP;

    switch (ret)
      {
      case QMessageBox::Ok:
        {
        m_Model->LoadToTP(fn_list.front().c_str(), fmt);
        return true;
        }
      case QMessageBox::Cancel:
      default:
        {
        return false;
        }
      }
    }
  else if (clicked == (QAbstractButton*)btnSeriesFromBegin)
    {
    m_Model->Load(fn_list, fmt);
    return true;
    }
  else if (clicked == (QAbstractButton*)btnSeriesFromTP)
    {
    m_Model->Load(fn_list, fmt, displayTP);
    return true;
    }

  return false;
}
