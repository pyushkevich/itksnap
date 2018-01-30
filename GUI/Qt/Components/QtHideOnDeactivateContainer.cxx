#include "QtHideOnDeactivateContainer.h"
#include <QEvent>

QtHideOnDeactivateContainer::QtHideOnDeactivateContainer(QWidget *parent)
    : QWidget(parent)
{

}

QtHideOnDeactivateContainer::~QtHideOnDeactivateContainer()
{

}

void QtHideOnDeactivateContainer::changeEvent(QEvent *event)
{
  QWidget::changeEvent(event);
  if(event->type() == QEvent::EnabledChange)
    {
    this->setVisible(this->isEnabled());
    }
}


