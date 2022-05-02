#include "MeshImportFileSelectionPage.h"
#include "ui_MeshImportFileSelectionPage.h"
#include "MeshImportModel.h"
#include "SNAPQtCommon.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "qfileinfo.h"

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
    return false;
    }

  std::vector<std::string> fn_list;

  for (auto fn : filenames)
    {
    fn_list.push_back(fn.toStdString());
    }

  std::sort(fn_list.begin(), fn_list.end());


  MeshImportModel::FileFormat fmt = m_Model->GetFileFormatByName(to_utf8(format));

  // Import the file
  m_Model->Load(fn_list, fmt);

  return true;
}
