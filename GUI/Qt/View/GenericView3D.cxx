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
  m_CursorPlacementStyle = vtkSmartPointer<CursorPlacementInteractorStyle>::New();

}

GenericView3D::~GenericView3D()
{
}

void GenericView3D::SetModel(Generic3DModel *model)
{
  m_Model = model;

  // Assign the renderer
  this->SetRenderer(m_Model->GetRenderer());

  // Pass the model to the placement style, which handles picking
  m_CursorPlacementStyle->SetModel(model);

  // Listen to updates on the model
  connectITK(m_Model, ModelUpdateEvent());

  // TODO: move this out of the qt class
  // Assign a point picker
  vtkSmartPointer<vtkWorldPointPicker> worldPointPicker =
    vtkSmartPointer<vtkWorldPointPicker>::New();
  m_Model->GetRenderer()->GetRenderWindowInteractor()->SetPicker(worldPointPicker);

  // Assign an interactor style
  // m_Renderer->GetRenderWindowInteractor()->SetInteractorStyle(m_CursorPlacementStyle);
  m_Model->GetRenderer()->GetRenderWindowInteractor()->SetInteractorStyle(
        vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());

}

void GenericView3D::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();
  m_Renderer->Update();
  this->repaint();
}

