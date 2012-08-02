#include "GenericView3D.h"
#include "Generic3DModel.h"
#include "Generic3DRenderer.h"

#include "vtkGenericRenderWindowInteractor.h"
#include <QEvent>
#include <QMouseEvent>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkCommand.h>
#include <QtVTKInteractionDelegateWidget.h>
#include <vtkWorldPointPicker.h>
#include <vtkRendererCollection.h>
#include <vtkObjectFactory.h>


class CursorPlacementInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static CursorPlacementInteractorStyle* New();
  vtkTypeRevisionMacro(CursorPlacementInteractorStyle, vtkInteractorStyleTrackballCamera)

  irisGetSetMacro(Model, Generic3DModel *);

  virtual void OnLeftButtonDown()
  {
    this->Interactor->GetPicker()->Pick(this->Interactor->GetEventPosition()[0],
                                        this->Interactor->GetEventPosition()[1],
                                        0,  // always zero.
                                        this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
    Vector3d picked;
    this->Interactor->GetPicker()->GetPickPosition(picked.data_block());

    std::cout << "Picked value: " << picked[0] << " " << picked[1] << " " << picked[2] << std::endl;
    m_Model->SetCursorFromPickResult(picked);

    // Forward events
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
  }

private:
  Generic3DModel *m_Model;
};

vtkCxxRevisionMacro(CursorPlacementInteractorStyle, "$Revision: 1.1 $")
vtkStandardNewMacro(CursorPlacementInteractorStyle)

GenericView3D::GenericView3D(QWidget *parent) :
    QtAbstractOpenGLBox(parent)
{
  // Create the renderer
  m_Renderer = Generic3DRenderer::New();

  // Repaint ourselves when renderer updates
  connectITK(m_Renderer, ModelUpdateEvent());

  iren = vtkGenericRenderWindowInteractor::New();
  iren->SetRenderWindow(m_Renderer->GetRenderWindow());

  // context events
  m_Renderer->GetRenderWindow()->AddObserver(
        vtkCommand::WindowMakeCurrentEvent,
        this, &GenericView3D::RendererCallback);

  m_Renderer->GetRenderWindow()->AddObserver(
        vtkCommand::WindowIsCurrentEvent,
        this, &GenericView3D::RendererCallback);

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> inter =
      vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

  m_CursorPlacementStyle = vtkSmartPointer<CursorPlacementInteractorStyle>::New();

  vtkSmartPointer<vtkWorldPointPicker> worldPointPicker =
    vtkSmartPointer<vtkWorldPointPicker>::New();
  iren->SetPicker(worldPointPicker);


  iren->SetInteractorStyle(m_CursorPlacementStyle);
  iren->Initialize();

  // Create interaction delegate widget to handle this interaction
  QtVTKInteractionDelegateWidget *delegate =
      new QtVTKInteractionDelegateWidget(this);
  delegate->SetVTKInteractor(iren);
  this->AttachSingleDelegate(delegate);
}

GenericView3D::~GenericView3D()
{
  iren->Delete();

}

void GenericView3D::RendererCallback(
    vtkObject *src, unsigned long event, void *data)
{
  if(event == vtkCommand::WindowMakeCurrentEvent)
    {
    this->makeCurrent();
    }
  else if(event == vtkCommand::WindowIsCurrentEvent)
    {
    bool *result = static_cast<bool *>(data);
    *result = QGLContext::currentContext() == this->context();
    }
}

void GenericView3D::SetModel(Generic3DModel *model)
{
  m_Model = model;
  m_Renderer->SetModel(model);

  // Pass the model to the placement style, which handles picking
  m_CursorPlacementStyle->SetModel(model);

  // Listen to updates on the model
  connectITK(m_Model, ModelUpdateEvent());
}

AbstractRenderer * GenericView3D::GetRenderer() const
{
  return m_Renderer;
}

void GenericView3D::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();
  m_Renderer->Update();
  this->repaint();
}

void GenericView3D::resizeGL(int w, int h)
{
  QtAbstractOpenGLBox::resizeGL(w,h);
  iren->SetSize(w, h);
}
