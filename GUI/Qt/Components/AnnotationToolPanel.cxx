#include "AnnotationToolPanel.h"
#include "ui_AnnotationToolPanel.h"

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
