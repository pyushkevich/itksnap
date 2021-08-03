// issue #24: Add label smoothing feature

#include "SmoothLabelsDialog.h"
#include "ui_SmoothLabelsDialog.h"
#include "SmoothLabelsModel.h"
#include "VoxelChangeReportDialog.h"
#include "VoxelChangeReportModel.h"
#include "GlobalUIModel.h"
#include <unordered_set>

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include <QtAbstractItemViewCoupling.h>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>

SmoothLabelsDialog::SmoothLabelsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SmoothLabelsDialog)
{
  ui->setupUi(this);

  // Set up standard item model for label list view
  m_simodel = new QStandardItemModel(this);
  m_simodel->setColumnCount(2);

  // Set up a filter model for the label list view
  m_LabelListFilterModel = new QSortFilterProxyModel(this);
  m_LabelListFilterModel->setSourceModel(m_simodel);
  m_LabelListFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
  m_LabelListFilterModel->setFilterKeyColumn(-1);
  ui->lvLabels->setModel(m_LabelListFilterModel);

  // Set up parameter panel
  // -- Populate unit dropdown
  // -- Index: 0 => mm; 1 => vox
  ui->sigmaUnit->addItems(QStringList() << "mm" << "vox");

  // -- Set up sigma inputs
  // ---- Sigma inputs should be real numbers
  ui->sigmaX->setValidator(new QDoubleValidator(this));
  ui->sigmaY->setValidator(new QDoubleValidator(this));
  ui->sigmaZ->setValidator(new QDoubleValidator(this));
}

SmoothLabelsDialog::~SmoothLabelsDialog()
{
  delete ui;
}

// Derived checkable row traits to implement smoothing specific logic
class SmoothingLabelsRowTraits : public CheckableRowTraits<TwoColumnColorLabelToQSIMCouplingRowTraits>
{
public:
  SmoothingLabelsRowTraits() = default;
  static void updateRow(QList<QStandardItem *> items, LabelType label, const ColorLabel &cl)
  {
      CheckableRowTraits::updateRow(items, label, cl);
      // Background should always participated in smoothing
      if (label == 0)
        {
          items[0]->setCheckState(Qt::CheckState::Checked);
          items[0]->setCheckable(false);
          items[0]->setEnabled(false);
          items[0]->setToolTip("Current smoothing algorithm always involves background label");
        }
  }
};

void SmoothLabelsDialog::SetModel(SmoothLabelsModel *model)
{
  m_Model = model;

  // Couple label table view to the model
  makeMultiRowCoupling((QAbstractItemView *) (ui->lvLabels),
                       m_Model->GetCurrentLabelModel(),
                       SmoothingLabelsRowTraits());

  // Set resizing behavior
#if QT_VERSION >= 0x050000
  ui->lvLabels->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->lvLabels->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
#else
  ui->lvLabels->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
  ui->lvLabels->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
#endif
}

void SmoothLabelsDialog::setAllLabelCheckStates(Qt::CheckState chkState)
{
  QStandardItem *currentItem;
  for (auto i = 0; i < m_simodel->rowCount(); ++i)
    {
      currentItem = m_simodel->item(i);
      if (currentItem->isEnabled())
        currentItem->setCheckState(chkState);
    }
}

void SmoothLabelsDialog::on_btnSelectAll_clicked()
{
  setAllLabelCheckStates(Qt::Checked);
}

void SmoothLabelsDialog::on_btnClearAll_clicked()
{
  setAllLabelCheckStates(Qt::Unchecked);
}

void SmoothLabelsDialog::syncSigmas(int8_t dim, const QString &newText)
{
  QLineEdit *follower1, *follower2;
  // pos 1 = x; 2 = y; 3 = z;
  switch(dim)
    {
      case 1:
        follower1 = ui->sigmaY;
        follower2 = ui->sigmaZ;
      break;
      case 2:
        follower1 = ui->sigmaX;
        follower2 = ui->sigmaZ;
      break;
      case 3:
        follower1 = ui->sigmaX;
        follower2 = ui->sigmaY;
      break;
    default:
      return;
    }

  // update text
  if (!follower1->isModified() && !follower2->isModified())
    {
      follower1->setText(newText);
      follower2->setText(newText);
    }
}

void SmoothLabelsDialog::on_sigmaX_textEdited(const QString &newText)
{
  if (newText.isEmpty())
    ui->sigmaX->setModified(false);

  syncSigmas(1, newText);
}

void SmoothLabelsDialog::on_sigmaY_textEdited(const QString &newText)
{
  if (newText.isEmpty())
    ui->sigmaY->setModified(false);

  syncSigmas(2, newText);
}

void SmoothLabelsDialog::on_sigmaZ_textEdited(const QString &newText)
{
  if (newText.isEmpty())
    ui->sigmaZ->setModified(false);

  syncSigmas(3, newText);
}

void SmoothLabelsDialog::on_btnApply_clicked()
{
  // Get all checked label
  QAbstractItemModel *im = nullptr;
  QAbstractProxyModel *pm = dynamic_cast<QAbstractProxyModel*>(ui->lvLabels->model());
  if (pm)
    im = pm->sourceModel();

  QModelIndexList checked = im->match(im->index(0,0), Qt::CheckStateRole, Qt::Checked, -1);

  // std::cout << checked.count() << endl;
  std::vector<LabelType> labelsToSmooth;
  std::unordered_set<LabelType> labelSet;
  for(auto it = checked.begin(); it != checked.end(); ++it)
    {
      LabelType oneLabel = im->data(*it, Qt::UserRole).value<LabelType>();
      labelsToSmooth.push_back(oneLabel);
      labelSet.insert(oneLabel);
    }

  // Build the confirmation box
  QMessageBox confirmBox;
  QString msg;
  if (labelSet.size() < 1)
    msg = QString("No label is selected for smoothing!");
  else if (labelSet.size() == 2)
    {
      LabelType lb = 0;
      // get the non-zero label
      for (auto it = labelSet.cbegin(); it != labelSet.cend(); ++it)
        {
          if (*it != 0)
            {
              lb = *it;
              break;
            }
        }

      msg = QString("Smooth Label %1?").arg(lb);
    }
  else
    msg = QString("Smooth these %1 Labels?").arg(labelSet.size() - 1);

  confirmBox.setText(msg);
  confirmBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

  int ret = confirmBox.exec();

  // Build the result panel
  VoxelChangeReportDialog *report = new VoxelChangeReportDialog(this);
  report->SetModel(m_Model->GetParent()->GetVoxelChangeReportModel());
  report->setDescription("Voxel Changes after Smoothing");

  // Execute smoothing logic
  if (ret == QMessageBox::Ok)
    {
      report->setStartPoint();

      // Assemble sigma array
      std::vector<double> sigmaArr;
      sigmaArr.push_back(ui->sigmaX->text().toDouble());
      sigmaArr.push_back(ui->sigmaY->text().toDouble());
      sigmaArr.push_back(ui->sigmaZ->text().toDouble());

      // Get unit indicator
      // -- current index: 0 => mm; 1 => vox
      typedef SmoothLabelsModel::SigmaUnit SigmaUnit;
      SigmaUnit unit = ui->sigmaUnit->currentIndex() == 0 ? SigmaUnit::mm : SigmaUnit::vox;

      m_Model->Smooth(labelSet, sigmaArr, unit, ui->chkSmoothAllFrames->isChecked());

      report->showReport();
    }

  /*
  QDialog *resultDialog = new QDialog(this);
  resultDialog->setFixedSize(550, 281);


  QTreeWidget *resultTree = new QTreeWidget(resultDialog);
  resultTree->setColumnCount(5);
  QStringList hdr;
  hdr << "Frame" << "Label" << "Change" << "Before" << "After";
  resultTree->setHeaderLabels(hdr);
  resultTree->setAlternatingRowColors(true);

  QVBoxLayout *vlo = new QVBoxLayout(resultDialog);
  vlo->addWidget(resultTree);

  QDialogButtonBox *btnBox = new QDialogButtonBox(resultDialog);
  QPushButton *btnOK = new QPushButton("OK");
  btnBox->addButton(btnOK, QDialogButtonBox::AcceptRole);
  connect(btnOK, SIGNAL(clicked()), resultDialog, SLOT(accept()));
  QGridLayout *btnLayout = new QGridLayout();
  btnLayout->addWidget(btnBox);
  vlo->addLayout(btnLayout);

  QTreeWidgetItem *sample = new QTreeWidgetItem(resultTree);
  sample->setText(0, "1");
  sample->setText(1, "Label 3");
  sample->setText(2, "500");
  sample->setText(3, "1000");
  sample->setText(4, "1500");

  ret = resultDialog->exec();
  */
}

void SmoothLabelsDialog::on_btnClose_clicked()
{
  this->close();
}

void SmoothLabelsDialog::on_inLabelFilter_textChanged(const QString &arg)
{
  m_LabelListFilterModel->setFilterFixedString(arg);
}

void SmoothLabelsDialog::showEvent(QShowEvent *e)
{
  // Call parent method
  QDialog::showEvent(e);

  // If the widget is not currently showing, update it's state
  m_Model->UpdateOnShow();
}
