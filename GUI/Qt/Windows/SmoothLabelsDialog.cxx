// issue #24: Add label smoothing feature

#include "SmoothLabelsDialog.h"
#include "ui_SmoothLabelsDialog.h"
#include "SmoothLabelsModel.h"

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include <QtAbstractItemViewCoupling.h>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

SmoothLabelsDialog::SmoothLabelsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SmoothLabelsDialog)
{
  ui->setupUi(this);

  // Set up standard item model for label list view
  QStandardItemModel *simodel = new QStandardItemModel(this);
  simodel->setColumnCount(2);

  // Set up a filter model for the label list view
  m_LabelListFilterModel = new QSortFilterProxyModel(this);
  m_LabelListFilterModel->setSourceModel(simodel);
  m_LabelListFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
  m_LabelListFilterModel->setFilterKeyColumn(-1);
  ui->lvLabels->setModel(m_LabelListFilterModel);
}

SmoothLabelsDialog::~SmoothLabelsDialog()
{
  delete ui;
}

void SmoothLabelsDialog::SetModel(SmoothLabelsModel *model)
{
  m_Model = model;

  // Couple label list view to the model
  makeMultiRowCoupling((QAbstractItemView *) (ui->lvLabels),
                       m_Model->GetCurrentLabelModel(),
                       TwoColumnColorLabelToQSIMCouplingRowTraits());

  // Set resizing behavior
#if QT_VERSION >= 0x050000
  ui->lvLabels->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->lvLabels->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
#else
  ui->lvLabels->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
  ui->lvLabels->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
#endif
}

void SmoothLabelsDialog::on_btnApply_clicked()
{
  m_Model->Smooth();
}

void SmoothLabelsDialog::on_btnClose_clicked()
{
  this->close();
}

void SmoothLabelsDialog::showEvent(QShowEvent *e)
{
  // Call parent method
  QDialog::showEvent(e);

  // If the widget is not currently showing, update it's state
  m_Model->UpdateOnShow();
}
