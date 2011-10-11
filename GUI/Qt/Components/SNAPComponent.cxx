#include "SNAPComponent.h"
#include "LatentITKEventNotifier.h"
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include "SNAPUIFlag.h"

SNAPComponent::SNAPComponent(QWidget *parent) :
    QWidget(parent)
{
}

void
SNAPComponent
::connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot)
{
  LatentITKEventNotifier::connect(src, ev, this, slot);
}
