#include "LabelEditorDialog.h"
#include "ui_LabelEditorDialog.h"
#include <LabelEditorModel.h>

#include <QtComboBoxCoupling.h>
#include <QtLineEditCoupling.h>
#include <QtListWidgetCoupling.h>
#include <QtSliderCoupling.h>
#include <QtSpinBoxCoupling.h>
#include <QtDoubleSpinBoxCoupling.h>

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

  // TODO: this should be hadnled by the Default Domain traits
  DefaultWidgetValueTraits<LabelType, QComboBox> wt;
  ItemSetWidgetDomainTraits<
      ConcreteColorLabelPropertyModel::DomainType,
      QComboBox,
      ColorLabelToComboBoxWidgetTraits> dt;

  makeCoupling(ui->testCombo, m_Model->GetCurrentLabelModel(), wt, dt);

  // TODO: this should be hadnled by the Default Domain traits
  DefaultWidgetValueTraits<LabelType, QListWidget> wt2;
  ItemSetWidgetDomainTraits<
      ConcreteColorLabelPropertyModel::DomainType,
      QListWidget,
      ColorLabelToListWidgetTraits> dt2;

  makeCoupling(ui->listLabels, m_Model->GetCurrentLabelModel(), wt2, dt2);

  // Coupling for the description of the current label. Override the default
  // signal for this widget.
  makeCoupling(ui->inLabelDescription,
               m_Model->GetCurrentLabelDescriptionModel(),
               SIGNAL(editingFinished()));

  // Coupling for the ID of the current label
  makeCoupling(ui->inLabelId, m_Model->GetCurrentLabelIdModel());

  // Opacity (there are two controls)
  makeCoupling(ui->inLabelOpacitySlider,
               m_Model->GetCurrentLabelOpacityModel());

  makeCoupling(ui->inLabelOpacitySpinner,
               m_Model->GetCurrentLabelOpacityModel(),
               SIGNAL(editingFinished()));


}
