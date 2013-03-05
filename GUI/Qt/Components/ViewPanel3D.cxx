#include "ViewPanel3D.h"
#include "ui_ViewPanel3D.h"
#include "GlobalUIModel.h"
#include "Generic3DModel.h"
#include "itkCommand.h"
#include "IRISException.h"
#include <QMessageBox>


#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#ifdef RENDERTHREAD
class RenderThread : public QThread
{
  Q_OBJECT

public:
  RenderThread(QObject *parent = 0);
  ~RenderThread();

  /** We call this when the segmentation image has changed or when the
   * user presses the update button */
  void UpdateScene();

signals:

  void updatedScene();

protected:

  void run();

private:

  QMutex mutex;
  QWaitCondition condition;
  bool restart;
  bool abort;


};


RenderThread::RenderThread(QObject *parent)
  : QThread(parent)
{
  restart = false;
  abort = false;
}

RenderThread::~RenderThread()
{
  mutex.lock();
  abort = true;
  condition.wakeOne();
  mutex.unlock();

  wait();
}

void RenderThread::UpdateScene()
{
  QMutexLocker locker(&mutex);
  if (!isRunning())
    {
    start(LowPriority);
    }
  else
    {
    restart = true;
    condition.wakeOne();
    }
}

void RenderThread::run()
{
  // What do we do?
  // m_Model
}

#endif


ViewPanel3D::ViewPanel3D(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ViewPanel3D)
{
  ui->setupUi(this);
}

ViewPanel3D::~ViewPanel3D()
{
  delete ui;
}

void ViewPanel3D::OnRenderProgress()
{
  // TODO: fix this, add progress bar
  // std::cout << "." << std::flush;
}

void ViewPanel3D::on_btnUpdateMesh_clicked()
{
  // Do something about a progress bar
  typedef itk::SimpleMemberCommand<ViewPanel3D> CommandType;
  SmartPtr<CommandType> cmd = CommandType::New();
  cmd->SetCallbackFunction(this, &ViewPanel3D::OnRenderProgress);

  try
    {
    // Tell the model to update itself
    m_Model->UpdateSegmentationMesh(cmd);
    }
  catch(IRISException & IRISexc)
    {
    QMessageBox::warning(this, "Problem generating mesh", IRISexc.what());
    }

  // TODO: Delete this later - should be automatic!
  ui->view3d->repaint();
}

void ViewPanel3D::Initialize(GlobalUIModel *globalUI)
{
  // Save the model
  m_GlobalUI = globalUI;
  m_Model = globalUI->GetModel3D();
  ui->view3d->SetModel(m_Model);
}


