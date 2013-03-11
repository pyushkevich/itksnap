#include "QtIPCManager.h"
#include "SNAPEvents.h"
#include "SynchronizationModel.h"


QtIPCManager::QtIPCManager(QWidget *parent) :
  SNAPComponent(parent)
{
  // Start the IPC timer at 30ms intervals
  startTimer(30);
}

void QtIPCManager::SetModel(SynchronizationModel *model)
{
  m_Model = model;

  // Listen to update events from the model
  connectITK(m_Model, ModelUpdateEvent());
}

void QtIPCManager::onModelUpdate(const EventBucket &bucket)
{
  // Update the model - this is where all the broadcasting takes place
  m_Model->Update();
}

void QtIPCManager::timerEvent(QTimerEvent *)
{
  if(!m_Model) return;
  m_Model->ReadIPCState();
}


