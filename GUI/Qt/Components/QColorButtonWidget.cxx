#include "QColorButtonWidget.h"
#include <QToolButton>
#include <QColorDialog>
#include <QHBoxLayout>
#include <SNAPQtCommon.h>

QColorButtonWidget::QColorButtonWidget(QWidget *parent) : QWidget(parent)
{
  m_Button = new QToolButton(this);
  m_Button->setText("Choose ...");
  m_Button->setIcon(CreateColorBoxIcon(16,16,m_value));
  m_Button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_Button->setIconSize(QSize(16,16));

  QHBoxLayout *lo = new QHBoxLayout();
  lo->setContentsMargins(0,0,0,0);
  lo->addWidget(m_Button);
  this->setLayout(lo);

  connect(m_Button, SIGNAL(clicked()), SLOT(onButtonPress()));
}

void QColorButtonWidget::setValue(QColor value)
{
  m_value = value;
  m_Button->setIcon(CreateColorBoxIcon(16,16,value));
  emit valueChanged();
}

void QColorButtonWidget::onButtonPress()
{
  QColor color = QColorDialog::getColor(m_value, this);
  if(color.isValid())
    setValue(color);
}
