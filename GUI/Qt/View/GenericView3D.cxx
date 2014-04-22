#include "GenericView3D.h"
#include "Generic3DModel.h"
#include "Generic3DRenderer.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "vtkGenericRenderWindowInteractor.h"
#include <QEvent>
#include <QMouseEvent>
#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleUser.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkCommand.h>
#include <QtVTKInteractionDelegateWidget.h>
#include <vtkPointPicker.h>
#include <vtkRendererCollection.h>
#include <vtkObjectFactory.h>
#include <vtkInteractorStyleSwitch.h>
#include "Window3DPicker.h"
#include "IRISApplication.h"

class CursorPlacementInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static CursorPlacementInteractorStyle* New();
  vtkTypeRevisionMacro(CursorPlacementInteractorStyle, vtkInteractorStyleTrackballCamera)

  irisGetSetMacro(Model, Generic3DModel *)

  virtual void OnLeftButtonDown()
  {
    if(!m_Model->PickSegmentationVoxelUnderMouse(
         this->Interactor->GetEventPosition()[0],
         this->Interactor->GetEventPosition()[1]))
      {
      // Forward events
      vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
      }
  }

private:
  Generic3DModel *m_Model;
};

class SpraycanInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static SpraycanInteractorStyle* New();
  vtkTypeRevisionMacro(SpraycanInteractorStyle, vtkInteractorStyleTrackballCamera)

  irisGetSetMacro(Model, Generic3DModel *)

  virtual void OnLeftButtonDown()
  {
    // Spray a voxel
    if(m_Model->SpraySegmentationVoxelUnderMouse(
         this->Interactor->GetEventPosition()[0],
         this->Interactor->GetEventPosition()[1]))
      {
      m_IsPainting = true;
      }
    else
      {
      // Forward events
      vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
      }
  }

  virtual void OnLeftButtonUp()
  {
    if(m_IsPainting)
      m_IsPainting = false;
    else
      vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
  }

  virtual void OnMouseMove()
  {
    if(m_IsPainting)
      m_Model->SpraySegmentationVoxelUnderMouse(
               this->Interactor->GetEventPosition()[0],
               this->Interactor->GetEventPosition()[1]);
    else
      vtkInteractorStyleTrackballCamera::OnMouseMove();
  }

protected:

  SpraycanInteractorStyle() : m_IsPainting(false), m_Model(NULL) {}
  virtual ~SpraycanInteractorStyle() {}

private:
  Generic3DModel *m_Model;
  bool m_IsPainting;
};


class ScalpelInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static ScalpelInteractorStyle* New();
  vtkTypeRevisionMacro(ScalpelInteractorStyle, vtkInteractorStyleTrackballCamera)

  irisGetSetMacro(Model, Generic3DModel *)

  virtual void OnLeftButtonDown()
  {
    Vector2i xevent(this->Interactor->GetEventPosition());
    switch(m_Model->GetScalpelStatus())
      {
      case Generic3DModel::SCALPEL_LINE_NULL:
        // Holding and dragging the LMB acts as a trackball, only clicking
        // the LMB causes drawing operations
        m_ClickStart = xevent;
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
        // m_Model->SetScalpelStartPoint(xevent[0], xevent[1]);
        break;
      case Generic3DModel::SCALPEL_LINE_STARTED:
        m_Model->SetScalpelEndPoint(xevent[0], xevent[1], true);
        break;
      case Generic3DModel::SCALPEL_LINE_COMPLETED:
        vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
        break;
      }
  }

  virtual void OnMouseMove()
  {
    Vector2i xevent(this->Interactor->GetEventPosition());
    switch(m_Model->GetScalpelStatus())
      {
      case Generic3DModel::SCALPEL_LINE_STARTED:
        m_Model->SetScalpelEndPoint(xevent[0], xevent[1], false);
        break;
      default:
        vtkInteractorStyleTrackballCamera::OnMouseMove();
        break;
      }
 }

  virtual void OnLeftButtonUp()
  {
    Vector2i xevent(this->Interactor->GetEventPosition());
    Vector2i delta = xevent - m_ClickStart;

    // Detect a click, which starts scalpel drawing
    if(m_Model->GetScalpelStatus() == Generic3DModel::SCALPEL_LINE_NULL)
      {
      if(delta.squared_magnitude() < 4)
        {
        m_Model->SetScalpelStartPoint(xevent[0], xevent[1]);
        }
      }

    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
  }

  virtual void OnEnter()
  {
    vtkInteractorStyleTrackballCamera::OnEnter();

    // Record that we're inside
    m_Inside = true;

    // Record the end point
    OnMouseMove();
  }

  virtual void OnLeave()
  {
    vtkInteractorStyleTrackballCamera::OnLeave();

    // Record that we're inside
    m_Inside = false;
  }

  irisGetMacro(Inside,bool)

protected:

  ScalpelInteractorStyle() : m_Model(NULL) {}
  virtual ~ScalpelInteractorStyle() {}

  Generic3DModel *m_Model;

  Vector2i m_ClickStart;
  bool m_Inside;
};



vtkCxxRevisionMacro(CursorPlacementInteractorStyle, "$Revision: 1.1 $")
vtkStandardNewMacro(CursorPlacementInteractorStyle)

vtkCxxRevisionMacro(SpraycanInteractorStyle, "$Revision: 1.1 $")
vtkStandardNewMacro(SpraycanInteractorStyle)

vtkCxxRevisionMacro(ScalpelInteractorStyle, "$Revision: 1.1 $")
vtkStandardNewMacro(ScalpelInteractorStyle)



GenericView3D::GenericView3D(QWidget *parent) :
    QtVTKRenderWindowBox(parent)
{
  // Create the interactor styles for each mode
  m_InteractionStyle[TRACKBALL_MODE]
      = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

  m_InteractionStyle[CROSSHAIRS_3D_MODE]
      = vtkSmartPointer<CursorPlacementInteractorStyle>::New();

  m_InteractionStyle[SCALPEL_MODE]
      = vtkSmartPointer<ScalpelInteractorStyle>::New();

  m_InteractionStyle[SPRAYPAINT_MODE]
      = vtkSmartPointer<SpraycanInteractorStyle>::New();
}

GenericView3D::~GenericView3D()
{
}

void GenericView3D::SetModel(Generic3DModel *model)
{
  m_Model = model;

  // Assign the renderer
  this->SetRenderer(m_Model->GetRenderer());

  // Pass the model to the different interactors
  CursorPlacementInteractorStyle::SafeDownCast(
        m_InteractionStyle[CROSSHAIRS_3D_MODE])->SetModel(model);

  SpraycanInteractorStyle::SafeDownCast(
        m_InteractionStyle[SPRAYPAINT_MODE])->SetModel(model);

  ScalpelInteractorStyle::SafeDownCast(
        m_InteractionStyle[SCALPEL_MODE])->SetModel(model);

  // Listen to toolbar changes
  connectITK(m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3DModel(),
             ValueChangedEvent(), SLOT(onToolbarModeChange()));

  // Use the current toolbar settings
  this->onToolbarModeChange();
}

void GenericView3D::onToolbarModeChange()
{
  int mode = (int) m_Model->GetParentUI()->GetGlobalState()->GetToolbarMode3D();

  m_Model->GetRenderer()->GetRenderWindowInteractor()
      ->SetInteractorStyle(m_InteractionStyle[mode]);

  setMouseTracking(mode == SCALPEL_MODE);
}

