#ifndef QTIPCMANAGER_H
#define QTIPCMANAGER_H

#include <QObject>
#include <SNAPComponent.h>

class SynchronizationModel;

/**
 * @brief This class manages IPC communications between SNAP sessions on the
 * GUI level. It uses Qt's timers to schedule checks for IPC updates, and it
 * listens to the events from the model layer in order to send IPC messages
 * out.
 */
class QtIPCManager : public SNAPComponent
{
  Q_OBJECT
public:
  explicit QtIPCManager(QWidget *parent = 0);

  void SetModel(SynchronizationModel *model);
  
signals:
  
public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

protected:

  virtual void timerEvent(QTimerEvent *);

private:

  SynchronizationModel *m_Model;
};

#endif // QTIPCMANAGER_H
