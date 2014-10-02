#include "MeshExportBrowsePage.h"
#include "ui_MeshExportBrowsePage.h"

#include "MeshExportModel.h"
#include "QtComboBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include <QFileInfo>
#include <QMenu>
#include <QFileDialog>
#include "SNAPQtCommon.h"
#include <QtCursorOverride.h>

Q_DECLARE_METATYPE(GuidedMeshIO::FileFormat)

MeshExportBrowsePage::MeshExportBrowsePage(QWidget *parent) :
  QWizardPage(parent),
  ui(new Ui::MeshExportBrowsePage)
{
  ui->setupUi(this);

  // Connect changes to the filename
  QObject::connect(ui->filePanel, SIGNAL(absoluteFilenameChanged(QString)),
                   this, SIGNAL(completeChanged()));
}

MeshExportBrowsePage::~MeshExportBrowsePage()
{
  delete ui;
}

void MeshExportBrowsePage::SetModel(MeshExportModel *model)
{
  // Store the model
  m_Model = model;
}

void MeshExportBrowsePage::initializePage()
{
  // Filter string (generate by hand...)
  QString filter;

  // Get the domain
  MeshExportModel::FileFormat dummy;
  MeshExportModel::FileFormatDomain domain;
  m_Model->GetExportFormatModel()->GetValueAndDomain(dummy, &domain);

  if(m_Model->GetSaveMode() == MeshExportModel::SAVE_SCENE)
    {
    filter = QString("%1 (.vtk);; %2 (.vrml)")
        .arg(from_utf8(domain[GuidedMeshIO::FORMAT_VTK]))
        .arg(from_utf8(domain[GuidedMeshIO::FORMAT_VRML]));
    }
  else
    {
    filter = QString("%1 (.vtk);; %2 (.stl);; %3 (.byu .y)")
        .arg(from_utf8(domain[GuidedMeshIO::FORMAT_VTK]))
        .arg(from_utf8(domain[GuidedMeshIO::FORMAT_STL]))
        .arg(from_utf8(domain[GuidedMeshIO::FORMAT_BYU]));
    }

  // Create the file panel
  ui->filePanel->initializeForSaveFile(
        m_Model->GetParentModel(),
        "Mesh file name:",
        from_utf8(m_Model->GetHistoryName()),
        filter, false,
        QString(), from_utf8(domain[GuidedMeshIO::FORMAT_VTK]));
}

bool MeshExportBrowsePage::validatePage()
{
  // Change cursor until this object moves out of scope
  QtCursorOverride curse(Qt::WaitCursor);

  try
  {
    m_Model->SetExportFileName(to_utf8(ui->filePanel->absoluteFilename()));
    m_Model->SetExportFormat(m_Model->GetFileFormatByName(to_utf8(ui->filePanel->activeFormat())));
    m_Model->SaveMesh();
    return true;
  }
  catch(std::exception &exc)
  {
    QString html_template =
        "<tr><td width=40><img src=\":/root/%1.png\" /></td>"
        "<td><p>%2</p></td></tr>";

    QString text = QString::fromUtf8(exc.what());
    QString head = text.section(".",0,0);
    QString tail = text.section(".", 1);

    QString html = QString("<table>%1</table>").arg(
          QString(html_template).arg(
            "dlg_error_32", QString("<b>%1.</b> %2").arg(head, tail)));

    ui->outMessage->setText(html);
    return false;
  }
}

bool MeshExportBrowsePage::isComplete()
{
  return ui->filePanel->absoluteFilename().length() > 0;
}
