#ifndef SYNCHRONIZATIONINSPECTOR_H
#define SYNCHRONIZATIONINSPECTOR_H

#include <QWidget>

namespace Ui {
class SynchronizationInspector;
}

class SynchronizationModel;

class SynchronizationInspector : public QWidget
{
  Q_OBJECT
  
public:
  explicit SynchronizationInspector(QWidget *parent = 0);
  ~SynchronizationInspector();

  void SetModel(SynchronizationModel *model);
  
private:
  Ui::SynchronizationInspector *ui;

  SynchronizationModel *m_Model;
};

#endif // SYNCHRONIZATIONINSPECTOR_H
