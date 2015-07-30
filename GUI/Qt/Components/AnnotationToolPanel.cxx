#include "AnnotationToolPanel.h"
#include "ui_AnnotationToolPanel.h"

#include "QtAbstractButtonCoupling.h"
#include "QtCheckableWidgetGroupCoupling.h"
#include "GlobalState.h"
#include "GlobalUIModel.h"
#include "QFileInfo"
#include "IRISApplication.h"

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
  mode_mapping[ANNOTATION_LANDMARK] = ui->btnText;
  mode_mapping[ANNOTATION_SELECT] = ui->btnSelect;
  makeCheckableWidgetGroupCoupling(ui->grpMode, mode_mapping,
                                   m_Model->GetGlobalState()->GetAnnotationModeModel());

  makeCoupling(ui->inColor, m_Model->GetGlobalState()->GetAnnotationColorModel());
}

void AnnotationToolPanel::on_btnOpen_clicked()
{
  // Load annotations
  QString file = ShowSimpleOpenDialogWithHistory(
                   this, m_Model, "Annotations", "Open Annotation File",
                   "Annotation File", "ITK-SNAP Annotation Files (*.annot)");

  if(!file.isNull())
    {
    QString file_abs = QFileInfo(file).absoluteFilePath();
    try
      {
      m_Model->GetDriver()->LoadAnnotations(to_utf8(file_abs).c_str());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Error Opening Annotation File",
                               QString("Failed to open annotation file %1").arg(file_abs));
      }
    }
}

void AnnotationToolPanel::on_btnSave_clicked()
{
  // Save annotations
  QString file = ShowSimpleSaveDialogWithHistory(
                   this, m_Model, "Annotations", "Open Annotation File",
                   "Annotation File", "ITK-SNAP Annotation Files (*.annot)", false);

  if(!file.isNull())
    {
    QString file_abs = QFileInfo(file).absoluteFilePath();
    try
      {
      m_Model->GetDriver()->SaveAnnotations(to_utf8(file_abs).c_str());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Error Saving Annotation File",
                               QString("Failed to save annotation file %1").arg(file_abs));
      }
    }

}
