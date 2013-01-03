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
    QtVTKRenderWindowBox(parent)
{
  // Create and assign the renderer
  m_Renderer = Generic3DRenderer::New();
  this->SetRenderer(m_Renderer);

  m_CursorPlacementStyle = vtkSmartPointer<CursorPlacementInteractorStyle>::New();

  // Assign a point picker
  vtkSmartPointer<vtkWorldPointPicker> worldPointPicker =
    vtkSmartPointer<vtkWorldPointPicker>::New();
  m_Renderer->GetRenderWindowInteractor()->SetPicker(worldPointPicker);

  // Assign an interactor style
  m_Renderer->GetRenderWindowInteractor()->SetInteractorStyle(m_CursorPlacementStyle);
}

GenericView3D::~GenericView3D()
{
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

void GenericView3D::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();
  m_Renderer->Update();
  this->repaint();
}

