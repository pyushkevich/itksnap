#include "MeshExportModePage.h"
#include "ui_MeshExportModePage.h"

#include "MeshExportModel.h"
#include "QtRadioButtonCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtWidgetActivator.h"

#include <map>

MeshExportModePage::MeshExportModePage(QWidget *parent) :
  QWizardPage(parent),
  ui(new Ui::MeshExportModePage)
{
  ui->setupUi(this);
}

MeshExportModePage::~MeshExportModePage()
{
  delete ui;
}

void MeshExportModePage::SetModel(MeshExportModel *model)
{
  m_Model = model;

  // Couple the widgets to the radio buttons
  std::map<MeshExportModel::SaveMode, QAbstractButton *> button_map;
  button_map[MeshExportModel::SAVE_SINGLE_LABEL] = ui->btnExportOne;
  button_map[MeshExportModel::SAVE_MULTIPLE_FILES] = ui->btnExportMultipleFiles;
  button_map[MeshExportModel::SAVE_SCENE] = ui->btnExportScene;
  makeRadioGroupCoupling(this, button_map, m_Model->GetSaveModeModel());

  // Couple the label widget
  makeCoupling(ui->inLabel, m_Model->GetExportedLabelModel());

  // Handle activation
  activateOnFlag(ui->inLabel, m_Model, MeshExportModel::UIF_LABEL_SELECTION_ACTIVE);
}

void MeshExportModePage::initializePage()
{
  m_Model->OnDialogOpen();
}
