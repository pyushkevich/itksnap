#include "LabelEditorDialog.h"
#include "ui_LabelEditorDialog.h"
#include <LabelEditorModel.h>
#include <QtListWidgetCoupling.h>

LabelEditorDialog::LabelEditorDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LabelEditorDialog)
{
  ui->setupUi(this);
}

LabelEditorDialog::~LabelEditorDialog()
{
  delete ui;
}

void LabelEditorDialog::SetModel(LabelEditorModel *model)
{
  // Store the model
  m_Model = model;

  // Couple widgets to the model

  DefaultWidgetValueTraits<LabelType, QComboBox> wt;
  ItemSetWidgetDomainTraits<
      ConcreteColorLabelPropertyModel::DomainType,
      QComboBox,
      ColorLabelToComboBoxWidgetTraits> dt;

  makeCoupling(ui->testCombo, m_Model->GetCurrentLabelModel(), wt, dt);

  DefaultWidgetValueTraits<LabelType, QListWidget> wt2;
  ItemSetWidgetDomainTraits<
      ConcreteColorLabelPropertyModel::DomainType,
      QListWidget,
      ColorLabelToListWidgetTraits> dt2;

  makeCoupling(ui->listLabels, m_Model->GetCurrentLabelModel(), wt2, dt2);

}
