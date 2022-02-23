#include "MeshImportFileSelectionPage.h"
#include "ui_MeshImportFileSelectionPage.h"
#include "MeshImportModel.h"
#include "SNAPQtCommon.h"

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

  /*
  connect(
        ui->filePanel, SIGNAL(absoluteFilenameChanged(QString)),
        this, SLOT(onFilenameChanged(QString)));
        */
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

  std::cout << "[MeshFileSelection] filter=" << filter.toStdString() << std::endl;

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
    ui->filePanel->initializeForOpenDirectory(
          m_Model->GetParentModel(),
          "Mesh file name: ",
          from_utf8(m_Model->GetHistoryName()),
          filter,
          QString(), from_utf8(domain[GuidedMeshIO::FORMAT_VTK]));
    }
}


bool MeshImportFileSelectionPage::validatePage()
{
  // Clear error state
  ui->lblMessage->clear();

  // Get selected format from the file panel
  QString format = ui->filePanel->activeFormat();
  QString filename = ui->filePanel->absoluteFilename();

  std::cout << "[MeshFileSelection] validatePage() format=" << format.toStdString() << std::endl;
  std::cout << "[MeshFileSelection] validatePage() filename=" << filename.toStdString() << std::endl;

  MeshImportModel::FileFormat fmt = m_Model->GetFileFormatByName(to_utf8(format));

  // Import the file
  m_Model->Load(filename.toUtf8(), fmt);

  return true;
}
