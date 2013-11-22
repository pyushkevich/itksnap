#ifndef VIEWPANEL3D_H
#define VIEWPANEL3D_H

#include <SNAPComponent.h>

namespace Ui {
  class ViewPanel3D;
}

class Generic3DModel;
class GenericView3D;

#include "Generic3DModel.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QTimer>
#include <itkCommand.h>

// Just playing around, this does not work
class RenderWorker : public QObject
{
  Q_OBJECT

public:
  RenderWorker(QObject *parent = 0)
    : QObject(parent), m_Model(NULL)
  {
    m_Timer = new QTimer();
    m_Timer->setInterval(1200);
    QObject::connect(m_Timer, SIGNAL(timeout()), this, SLOT(timerHit()), Qt::DirectConnection);

    m_Command = CommandType::New();
    m_Command->SetCallbackFunction(this, &RenderWorker::filter_cb);
  }

  void filter_cb() const
  {
    qDebug() << "progress ";
  }

  void SetModel(Generic3DModel *model)
  {
    m_Model = model;
  }

public slots:

  void timerHit()
  {
    // Update the mesh if necessary
    if(m_Model)
      {
      if(m_Model->CheckState(Generic3DModel::UIF_MESH_DIRTY))
        {
        m_Model->UpdateSegmentationMesh(m_Command);
        qDebug() << "Updated mesh in worker thread";
        emit updatedScene();
        }
      }
  }

  void process()
  {
    qDebug() << "Worker starting";
    m_Timer->start();   // puts one event in the threads event queue
  }


signals:

  void updatedScene();

protected:


  Generic3DModel *m_Model;
  QTimer *m_Timer;
  typedef itk::SimpleConstMemberCommand<RenderWorker> CommandType;
  SmartPtr<CommandType> m_Command;
};

class ViewPanel3D : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ViewPanel3D(QWidget *parent = 0);
  ~ViewPanel3D();

  // Register with the global model
  void Initialize(GlobalUIModel *model);

  GenericView3D *Get3DView();

private slots:
  virtual void onModelUpdate(const EventBucket &bucket);

  void on_btnUpdateMesh_clicked();

  void on_btnScreenshot_clicked();

  void on_btnResetView_clicked();

  void on_btnAccept_clicked();

  void on_btnCancel_clicked();

  void on_btnExpand_clicked();

  void onWorkerUpdate();

  void onTimer();


private:
  Ui::ViewPanel3D *ui;

  GlobalUIModel *m_GlobalUI;

  Generic3DModel *m_Model;

  RenderWorker *m_RenderWorker;

  QTimer *m_RenderTimer;

  void UpdateExpandViewButton();
};

#endif // VIEWPANEL3D_H
