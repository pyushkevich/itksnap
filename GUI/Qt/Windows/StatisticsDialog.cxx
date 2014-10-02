#include "StatisticsDialog.h"
#include "ui_StatisticsDialog.h"

#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SegmentationStatistics.h"
#include "HistoryManager.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QMimeData>
#include <QClipboard>
#include <SNAPQtCommon.h>
#include <QtCursorOverride.h>
#include <SimpleFileDialogWithHistory.h>

StatisticsDialog::StatisticsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::StatisticsDialog)
{
  ui->setupUi(this);
  this->setObjectName("dlgStatistics");
  m_ItemModel = new QStandardItemModel(this);
  ui->tvVolumes->setModel(m_ItemModel);
  m_Stats = new SegmentationStatistics();
}

StatisticsDialog::~StatisticsDialog()
{
  delete ui;
  delete m_Stats;
}

void StatisticsDialog::SetModel(GlobalUIModel *model)
{
  m_Model = model;
}

void StatisticsDialog::Activate()
{
  QtCursorOverride cursy(Qt::WaitCursor);
  this->FillTable();
  this->show();
  this->raise();
  this->activateWindow();
}

void StatisticsDialog::FillTable()
{
  // Compute the segmentation statistics
  m_Stats->Compute(m_Model->GetDriver()->GetCurrentImageData());

  // Fill out the item model
  m_ItemModel->clear();

  // Set the column names
  QStringList header;
  header << "Label Name" << "Voxel Count" << "Volume (mm3)";
  m_ItemModel->setHorizontalHeaderLabels(header);

  const std::vector<std::string> &cols = m_Stats->GetImageStatisticsColumns();
  for(int j = 0; j < cols.size(); j++)
    {
    QString label = QString("Intensity Mean %1 SD\n(%2)").arg(QChar(0x00B1)).arg(from_utf8(cols[j]));

    QStandardItem *item = new QStandardItem();
    item->setText(label);
    item->setToolTip(
          QString("Mean intensity and standard deviation for layer %1").arg(from_utf8(cols[j])));

    m_ItemModel->setHorizontalHeaderItem(j+3, item);
    }

  // Add all the rows
  for(SegmentationStatistics::EntryMap::const_iterator it = m_Stats->GetStats().begin();
      it != m_Stats->GetStats().end(); ++it)
    {
    LabelType i = it->first;
    const SegmentationStatistics::Entry &row = it->second;
    const ColorLabel &cl = m_Model->GetDriver()->GetColorLabelTable()->GetColorLabel(i);
    if(row.count)
      {
      QList<QStandardItem *> qsi;
      QIcon icon = CreateColorBoxIcon(16,16, QColor(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
      qsi.append(new QStandardItem(icon, cl.GetLabel()));
      qsi.append(new QStandardItem(QString("%1").arg(row.count)));
      qsi.append(new QStandardItem(QString("%1").arg(row.volume_mm3,0,'g',4)));
      for(int j = 0; j < row.mean.size(); j++)
        {
        QString text = QString("%1%2%3")
            .arg(row.mean[j],0,'f',4)
            .arg(QChar(0x00B1))
            .arg(row.stdev[j],0,'f',4);
        qsi.append(new QStandardItem(text));
        }
      m_ItemModel->appendRow(qsi);
      m_ItemModel->setVerticalHeaderItem(m_ItemModel->rowCount()-1,
                                         new QStandardItem(QString("%1").arg(i)));
      }
    }

  // Perform a smart resize of the column widts
  ui->tvVolumes->resizeColumnsToContents();
  for(int col = 0; col < ui->tvVolumes->horizontalHeader()->count(); col++)
    {
    if(ui->tvVolumes->horizontalHeader()->sectionSize(col) > 150)
      ui->tvVolumes->horizontalHeader()->resizeSection(col, 150);
    }
}

void StatisticsDialog::on_btnUpdate_clicked()
{
  QtCursorOverride cursy(Qt::WaitCursor);
  this->FillTable();
}


void StatisticsDialog::on_btnCopy_clicked()
{
  std::ostringstream oss;
  m_Stats->Export(oss, "\t", *m_Model->GetDriver()->GetColorLabelTable());
  QString tsv = QString::fromStdString(oss.str());

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(tsv);
}

#include <fstream>

void StatisticsDialog::on_btnExport_clicked()
{
  // Ask for a filename
  QString selection = ShowSimpleSaveDialogWithHistory(
        this, m_Model, "Statistics",
        "Export Volumes and Statistics - ITK-SNAP",
        "Volumes and Statistics File",
        "Text Files (*.txt);; Comma Separated Value Files (*.csv)",
        true);

  // Open the labels from the selection
  if(selection.length())
    {
    try
      {
      std::ofstream fout(selection.toUtf8());
      if(selection.endsWith(".csv", Qt::CaseInsensitive))
        {
        m_Stats->Export(fout, ",", *m_Model->GetDriver()->GetColorLabelTable());
        }
      else
        {
        m_Stats->Export(fout, "\t", *m_Model->GetDriver()->GetColorLabelTable());
        }
      m_Model->GetSystemInterface()->GetHistoryManager()->
          UpdateHistory("Statistics", to_utf8(selection), true);
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Volume and Statistics IO Error",
                               QString("Failed to export volumes and statistics"));
      }
    }
}
