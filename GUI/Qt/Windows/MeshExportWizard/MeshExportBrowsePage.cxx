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

  // Create a menu for the history button
  QMenu *history_menu = new QMenu("History");
  history_menu->setStyleSheet("font-size: 12px;");
  ui->btnHistory->setMenu(history_menu);

  // Completion signals
  QObject::connect(ui->inFilename, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
}

MeshExportBrowsePage::~MeshExportBrowsePage()
{
  delete ui;
}

void MeshExportBrowsePage::SetModel(MeshExportModel *model)
{
  // Store the model
  m_Model = model;

  // Make a coupling for the filename field
  makeCoupling(ui->inFilename, m_Model->GetExportFileNameModel());
  makeCoupling(ui->inFormat, m_Model->GetExportFormatModel());
}

void MeshExportBrowsePage::initializePage()
{
  // Fill out the history menu
  PopulateHistoryMenu(ui->btnHistory->menu(),
                      this, SLOT(on_historySelection()),
                      m_Model->GetParentModel(),
                      from_utf8(m_Model->GetHistoryName()));
  ui->btnHistory->setEnabled(ui->btnHistory->menu()->actions().size() > 0);
}

bool MeshExportBrowsePage::validatePage()
{
  // Change cursor until this object moves out of scope
  QtCursorOverride curse(Qt::WaitCursor);

  try
  {
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

void MeshExportBrowsePage::on_btnBrowse_clicked()
{
  // Initialize the dialog with what's in the filebox
  QString filename = ui->inFilename->text();

  // Set the dialog properties
  QFileInfo fi(filename);

  // Run the dialog
  QString sel = QFileDialog::getSaveFileName(this, "Export Mesh As", fi.path());

  if(sel.length())
    ui->inFilename->setText(sel);
}

void MeshExportBrowsePage::on_historySelection()
{
  QAction *action = static_cast<QAction *>(this->sender());
  ui->inFilename->setText(action->text());
}

bool MeshExportBrowsePage::isComplete()
{
  return ui->inFilename->text().size() > 0;
}
