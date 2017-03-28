#ifndef _TestOpenGLDialog_
#define _TestOpenGLDialog_

#include <QtVTKRenderWindowBox.h>
#include <AbstractVTKRenderer.h>
#include <QApplication>
#include <QMouseEvent>
#include <QDebug>

#include <vtkSphereSource.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkDiskSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCommand.h>

class SillyRenderer : public AbstractVTKRenderer 
{
public:
  irisITKObjectMacro(SillyRenderer, AbstractVTKRenderer)

protected:
  SillyRenderer()
    {
    // Create a sphere
    vtkSmartPointer<vtkSphereSource> sphereSource = 
      vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetCenter(0.0, 0.0, 0.0);
    sphereSource->SetRadius(5.0);
   
    vtkSmartPointer<vtkPolyDataMapper> mapper = 
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
   
    vtkSmartPointer<vtkActor> actor = 
      vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkDiskSource> diskSource =
      vtkSmartPointer<vtkDiskSource>::New();

    // Create a mapper and actor.
    vtkSmartPointer<vtkPolyDataMapper> mapper2 =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper2->SetInputConnection(diskSource->GetOutputPort());

    vtkSmartPointer<vtkActor> actor2 =
      vtkSmartPointer<vtkActor>::New();
    actor2->SetMapper(mapper2);

    // Setup the text and add it to the renderer
     vtkSmartPointer<vtkTextActor> textActor =
       vtkSmartPointer<vtkTextActor>::New();
     textActor->SetInput ( "Hello world" );
     textActor->SetPosition2 ( 10, 40 );
     textActor->GetTextProperty()->SetFontSize ( 24 );
     textActor->GetTextProperty()->SetColor ( 1.0, 0.0, 0.0 );
     m_Renderer->AddActor2D ( textActor );

   
    // m_Renderer->AddActor(actor);
    m_Renderer->AddActor(actor2);
    m_Renderer->SetBackground(.3, .6, .3); // Background color green
    }

  virtual ~SillyRenderer() {}

  friend class TestOpenGLDialog;
};


class TestOpenGLDialog : public QOpenGLWidget
{
  Q_OBJECT

public:
  explicit TestOpenGLDialog(QWidget *w = NULL) 
  : QOpenGLWidget(w)
    {
    m_Silly = SillyRenderer::New();
    m_RenWin = vtkGenericOpenGLRenderWindow::New();
    m_RenWin->AddRenderer(m_Silly->m_Renderer);
    m_RenWin->SwapBuffersOff();

    m_Interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_Interactor->SetRenderWindow(m_RenWin);

    vtkSmartPointer<vtkInteractorObserver> is 
      = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

    m_Interactor->SetInteractorStyle(is);
    m_Interactor->Initialize();
    m_Interactor->Enable();
    m_Interactor->EnableRenderOff();


    // Hook up context-related events (is this needed?)
    m_RenWin->AddObserver(
          vtkCommand::WindowMakeCurrentEvent,
          this, &TestOpenGLDialog::RendererCallback);

    m_RenWin->AddObserver(
          vtkCommand::WindowIsCurrentEvent,
          this, &TestOpenGLDialog::RendererCallback);

    m_RenWin->AddObserver(
          vtkCommand::WindowFrameEvent,
          this, &TestOpenGLDialog::RendererCallback);

    m_Interactor->AddObserver(
          vtkCommand::RenderEvent,
          this, &TestOpenGLDialog::RendererCallback);
    }

void RendererCallback(
    vtkObject *src, unsigned long event, void *data)
{
  /*
  if(event == vtkCommand::WindowMakeCurrentEvent)
    {
    std::cout << 1 << std::endl;
    this->makeCurrent();
    }
  else if(event == vtkCommand::WindowIsCurrentEvent)
    {
    std::cout << 2 << std::endl;
    bool *result = static_cast<bool *>(data);
    *result = QOpenGLContext::currentContext() == this->context();
    }
  else if(event == vtkCommand::WindowFrameEvent)
    {
    std::cout << 3 << std::endl;
    // if(m_RenWin->GetSwapBuffers())
      this->context()->swapBuffers(this->context()->surface());
    }
  else */
  if(event == vtkCommand::RenderEvent)
    {
    std::cout << "Received Render Event" << std::endl;
    this->update();
    }
}

  virtual void paintGL()
  {
    // Clear the screen
    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::cout << "Paint Begin" << std::endl;
    m_RenWin->Render();
    std::cout << "Paint End" << std::endl;
  }

#if QT_VERSION < 0x050000
  virtual int devicePixelRatio()
  {
    return 1;
  }
#endif

  virtual void resizeGL(int w, int h)
  {
    int dpr = this->devicePixelRatio();
    m_RenWin->SetSize(w * dpr, h * dpr);

    qDebug() << this->format();
  }

  virtual void initializeGL()
  {
    // Is this what we should be calling?
    m_RenWin->OpenGLInit();
  }
  
void SetVTKEventState(QMouseEvent *ev)
{
  Qt::KeyboardModifiers km = QApplication::keyboardModifiers();

  // Account for Retina displays
  int x = ev->pos().x() * this->devicePixelRatio();
  int y = ev->pos().y() * this->devicePixelRatio();

  m_Interactor->SetEventInformationFlipY(
        x, y, 
        km.testFlag(Qt::ControlModifier),
        km.testFlag(Qt::ShiftModifier));
}
void mousePressEvent(QMouseEvent *ev)
{
  // Set the position information
  SetVTKEventState(ev);

  // Fire appropriate event
  if(ev->button() == Qt::LeftButton)
    m_Interactor->LeftButtonPressEvent();
  else if(ev->button() == Qt::RightButton)
    m_Interactor->RightButtonPressEvent();
  else if(ev->button() == Qt::MiddleButton)
    m_Interactor->MiddleButtonPressEvent();

  // this->update();
}

void mouseReleaseEvent(QMouseEvent *ev)
{
  // Set the position information
  SetVTKEventState(ev);

  // Fire appropriate event
  if(ev->button() == Qt::LeftButton)
    m_Interactor->LeftButtonReleaseEvent();
  else if(ev->button() == Qt::RightButton)
    m_Interactor->RightButtonReleaseEvent();
  else if(ev->button() == Qt::MiddleButton)
    m_Interactor->MiddleButtonReleaseEvent();

  // this->update();
}

void mouseMoveEvent(QMouseEvent *ev)
{
  // Set the position information
  SetVTKEventState(ev);
  m_Interactor->MouseMoveEvent();

  // this->update();
}

  virtual ~TestOpenGLDialog() {}

private:
  SillyRenderer::Pointer m_Silly;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_RenWin;
  vtkSmartPointer<vtkRenderWindowInteractor> m_Interactor;
};

#endif
