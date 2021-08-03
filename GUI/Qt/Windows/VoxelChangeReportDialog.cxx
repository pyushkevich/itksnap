#include "VoxelChangeReportDialog.h"
#include "ui_VoxelChangeReportDialog.h"
#include "VoxelChangeReportModel.h"
#include "ColorLabelTable.h"
#include "ColorLabel.h"
#include "SNAPQtCommon.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>


VoxelChangeReportDialog::VoxelChangeReportDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::VoxelChangeReportDialog)
{
  ui->setupUi(this);
}

VoxelChangeReportDialog::~VoxelChangeReportDialog()
{
  delete ui;
}

void VoxelChangeReportDialog::setStartPoint()
{
  m_Model->setStartingPoint();
}

void VoxelChangeReportDialog::setDescription(QString des)
{
  ui->description->setText(des);
}

void VoxelChangeReportDialog::showReport()
{
  // Get report from the model
  typedef VoxelChangeReportModel::VoxelChangeReportType ReportType;
  ReportType &report = m_Model->getReport();

  // Build the table
  QTreeWidget *tree = ui->tree;

  // -- configure columns
  tree->setColumnCount(4);
  QStringList header;
  header << "Frame" << "Label" << "Original Volume (mm3)" << "Volume Change %";
  tree->setColumnWidth(0, 50);
  tree->setColumnWidth(1, 70);
  tree->setColumnWidth(2, 175);
  tree->setColumnWidth(3, 150);
  tree->setHeaderLabels(header);
  tree->setAlternatingRowColors(true);

  // Get label color tabel
  ColorLabelTable *clTable = m_Model->GetColorLabelTable();

  // -- add items to the tree
  for (auto fit = report.cbegin(); fit != report.cend(); ++fit)
    {
      if (fit->second.size() > 1)
        {
          QTreeWidgetItem *frame = new QTreeWidgetItem(tree);
          frame->setText(0, QString::number(fit->first + 1)); // frame
          for (auto lit = fit->second.cbegin(); lit != fit->second.cend(); ++lit)
            {
              if (lit->second->cnt_change != 0)
                {
                  const ColorLabel &cl = clTable->GetColorLabel(lit->first);
                  QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));
                  QIcon ic = CreateColorBoxIcon(16, 16, fill);

                  QTreeWidgetItem *labelLine = new QTreeWidgetItem(frame);
                  //labelLine->setText(0, QString::number(fit->first + 1)); // frame
                  labelLine->setText(1, QString::number(lit->first)); // label
                  labelLine->setIcon(1, ic); // label color
                  labelLine->setText(2, QString::number(lit->second->vol_before_mm3)); // volume before
                  labelLine->setText(3, QString{"%1%"}.arg(lit->second->vol_change_pct)); // volume change %
                }
            }
        }
    }

  /*
  // Always expand the first item
  if (firstItem)
    {
      connect(this, SIGNAL(finished(0)), tree, SLOT(expandItem(firstItem)));
      emit(this->finished(0));
    }
  */


  // Render the GUI
  this->exec();
}



void VoxelChangeReportDialog::SetModel(VoxelChangeReportModel *model)
{
  this->m_Model = model;
}
