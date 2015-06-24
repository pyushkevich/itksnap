#include "AnnotationToolPanel.h"
#include "ui_AnnotationToolPanel.h"

#include "QtAbstractButtonCoupling.h"
#include "QtCheckableWidgetGroupCoupling.h"
#include "GlobalState.h"
#include "GlobalUIModel.h"

AnnotationToolPanel::AnnotationToolPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::AnnotationToolPanel)
{
  ui->setupUi(this);
}

AnnotationToolPanel::~AnnotationToolPanel()
{
  delete ui;
}

void AnnotationToolPanel::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  std::map<AnnotationMode, QAbstractButton *> mode_mapping;
  mode_mapping[ANNOTATION_RULER] = ui->btnRuler;
  mode_mapping[ANNOTATION_SELECT] = ui->btnSelect;
  makeCheckableWidgetGroupCoupling(ui->grpMode, mode_mapping,
                                   m_Model->GetGlobalState()->GetAnnotationModeModel());
}
