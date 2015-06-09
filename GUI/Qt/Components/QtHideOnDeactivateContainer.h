#ifndef QTHIDEONDEACTIVATECONTAINER_H
#define QTHIDEONDEACTIVATECONTAINER_H

#include <QWidget>

class QtHideOnDeactivateContainer : public QWidget
{
  Q_OBJECT
public:
  QtHideOnDeactivateContainer(QWidget *parent);
  ~QtHideOnDeactivateContainer();

  virtual void changeEvent(QEvent *event);
};

#endif // QTHIDEONDEACTIVATECONTAINER_H
