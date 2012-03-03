#include "ColorMapInspector.h"
#include "ui_ColorMapInspector.h"

ColorMapInspector::ColorMapInspector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorMapInspector)
{
    ui->setupUi(this);
}

ColorMapInspector::~ColorMapInspector()
{
    delete ui;
}
