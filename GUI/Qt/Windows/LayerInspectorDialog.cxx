#include "LayerInspectorDialog.h"
#include "ui_LayerInspectorDialog.h"
#include "ContrastInspector.h"

LayerInspectorDialog::LayerInspectorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerInspectorDialog)
{
    ui->setupUi(this);
}

LayerInspectorDialog::~LayerInspectorDialog()
{
  delete ui;
}

ContrastInspector * LayerInspectorDialog::GetContrastInspector()
{
  return ui->cmpInspector;
}
