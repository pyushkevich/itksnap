#include "AnnotationEditDialog.h"
#include "ui_AnnotationEditDialog.h"

#include "AnnotationModel.h"

#include "QtLineEditCoupling.h"
#include "TagListWidgetCoupling.h"
#include <QtAbstractButtonCoupling.h>

AnnotationEditDialog::AnnotationEditDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AnnotationEditDialog)
{
  ui->setupUi(this);
}

void AnnotationEditDialog::SetModel(AnnotationModel *model)
{
  this->m_Model = model;

  // Make connections with editors
  QtCouplingOptions opts_elt(QtCouplingOptions::DEACTIVATE_WHEN_INVALID);
  makeCoupling(ui->inLandmarkText, m_Model->GetSelectedLandmarkTextModel(), opts_elt);
  makeCoupling(ui->inLandmarkTag, m_Model->GetSelectedAnnotationTagsModel(), opts_elt);
  makeCoupling(ui->inLandmarkColor, m_Model->GetSelectedAnnotationColorModel(), opts_elt);
}

