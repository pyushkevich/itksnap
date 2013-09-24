#include "CollapsableGroupBox.h"
#include "ui_CollapsableGroupBox.h"
#include "QLayout"

CollapsableGroupBox::CollapsableGroupBox(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::CollapsableGroupBox)
{
  ui->setupUi(this);
}

CollapsableGroupBox::~CollapsableGroupBox()
{
  delete ui;
}

QString CollapsableGroupBox::title() const
{
  return ui->pushButton->text();
}

void CollapsableGroupBox::setTitle(QString title)
{
  ui->pushButton->setText(title);
}

void CollapsableGroupBox::addWidget(QWidget *widget)
{
  ui->body->layout()->addWidget(widget);
}

void CollapsableGroupBox::collapse(bool flag)
{
  ui->body->setMaximumHeight(flag ? 0 : 999999);
}
