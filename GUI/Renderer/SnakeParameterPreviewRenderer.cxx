#include "SnakeParameterPreviewRenderer.h"
#include "SnakeParametersPreviewPipeline.h"
#include "SnakeParameterModel.h"

#include <vtkRenderWindow.h>
#include <vtkContextItem.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkContext2D.h>
#include <vtkTransform2D.h>
#include <vtkImageData.h>
#include <vtkBrush.h>
#include <vtkPen.h>
#include <vtkPoints2D.h>
#include <vtkContextDevice2D.h>
#include <vtkObjectFactory.h>
#include "SNAPExportITKToVTK.h"

class SnakeParameterContextItem : public vtkContextItem
{
public:
  vtkTypeMacro(SnakeParameterContextItem, vtkContextItem)
  static SnakeParameterContextItem *New();

  SnakeParameterContextItem()
  {
    m_ForceToDisplay = SnakeParameterPreviewRenderer::TOTAL_FORCE;
  }

  irisGetMacro(Model, SnakeParameterModel *)

  /** Set the display mode */
  irisSetMacro(ForceToDisplay, SnakeParameterPreviewRenderer::DisplayMode)

  void SetModel(SnakeParameterModel *model)
  {
    m_Model = model;
    m_Pipeline = model->GetPreviewPipeline();

    // Import the display image into a vtk Texture
    typedef SnakeParametersPreviewPipeline::DisplayImageType DisplayImageType;
    typedef itk::VTKImageExport<DisplayImageType> ExportType;
    ExportType::Pointer exporter = ExportType::New();
    exporter->SetInput(m_Pipeline->GetDisplayImage());
    vtkNew<vtkImageImport> importer;
    ConnectITKExporterToVTKImporter(exporter.GetPointer(), importer);
    importer->Update();
    m_Texture = importer->GetOutput();
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Update everything
    m_Pipeline->Update();
    if(!m_Model)
      return false;

    // Set up the model matrix
    vtkNew<vtkTransform2D> tran;
    int w = this->GetScene()->GetSceneWidth();
    int h = this->GetScene()->GetSceneHeight();
    int tw = m_Texture->GetDimensions()[0];
    int th = m_Texture->GetDimensions()[1];

    auto sz_img = m_Pipeline->GetSpeedImage()->GetBufferedRegion().GetSize();
    tran->Scale(w * 1.0 / sz_img[0], h * 1.0 / sz_img[1]);
    painter->PushMatrix();
    painter->SetTransform(tran);

    // Draw the speed image
    painter->GetBrush()->SetColorF(1,1,1);
    painter->GetBrush()->SetTexture(m_Texture);
    painter->GetPen()->SetLineType(vtkPen::NO_PEN);
    painter->DrawRect(0, 0, tw, th);
    painter->GetBrush()->SetTexture(nullptr);

    // Set up the line drawing mode
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);

    // TODO: VPPR
    int vppr = 1.0;
    painter->GetPen()->SetWidth(2.0 * vppr);
    painter->GetPen()->SetColorF(1.0, 0.0, 0.0);
    painter->GetPen()->SetOpacityF(0.75);

    // Draw the evolving contour if it's available
    if(m_Model->GetAnimateDemo())
      {
      painter->DrawLines(m_Pipeline->GetDemoLoopContour());
      painter->GetPen()->SetOpacityF(0.5);
      }

    // Get the point collection
    const auto &samples = m_Pipeline->GetSampledPoints();

    // Draw the spline
    vtkNew<vtkPoints2D> pSamples;
    vtkNew<vtkPoints2D> pForces;
    pSamples->Allocate(samples.size());
    pSamples->Allocate(samples.size() * 2);
    for(auto p: samples)
      {
      pSamples->InsertNextPoint(p.x[0] * tw, p.x[1] * th);

      // Decide which force to draw, depending on the current state
      double force = 0;
      switch(m_ForceToDisplay)
        {
        case SnakeParameterPreviewRenderer::PROPAGATION_FORCE :
          force = p.PropagationForce;
          break;
        case SnakeParameterPreviewRenderer::CURVATURE_FORCE :
          force = p.CurvatureForce;
          break;
        case SnakeParameterPreviewRenderer::ADVECTION_FORCE :
          force = p.AdvectionForce;
          break;
        case SnakeParameterPreviewRenderer::TOTAL_FORCE :
          force = p.PropagationForce + p.CurvatureForce + p.AdvectionForce;
          break;
        }

      // Scale the force for effect
      force *= 10;

      // Draw the forces
      pForces->InsertNextPoint(p.x[0] * tw, p.x[1] * th);
      pForces->InsertNextPoint(
            p.x[0] * tw + force * p.n[0],
            p.x[1] * th + force * p.n[1]);
      }

    // Draw the spline
    painter->DrawPoly(pSamples);

    // Draw the forces
    painter->DrawLines(pForces);

    // No more image scaling
    painter->PopMatrix();

    return true;
  }

private:
  SnakeParameterModel *m_Model;
  SnakeParametersPreviewPipeline *m_Pipeline;
  vtkSmartPointer<vtkImageData> m_Texture;
  SnakeParameterPreviewRenderer::DisplayMode m_ForceToDisplay;
};


vtkStandardNewMacro(SnakeParameterContextItem)


#include <vtkRenderer.h>
SnakeParameterPreviewRenderer::SnakeParameterPreviewRenderer()
  : AbstractVTKSceneRenderer()
{
  m_ContextItem = vtkSmartPointer<SnakeParameterContextItem>::New();
  this->GetScene()->AddItem(m_ContextItem);

  this->SetBackgroundColor(Vector3d(1, 0, 1));
}

SnakeParameterPreviewRenderer::~SnakeParameterPreviewRenderer()
{
}

void SnakeParameterPreviewRenderer::SetModel(SnakeParameterModel *model)
{
  m_Model = model;
  m_Pipeline = model->GetPreviewPipeline();

  // Assign the model to the context item
  m_ContextItem->SetModel(m_Model);

  // Events
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model->GetAnimateDemoModel(), ValueChangedEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model, SnakeParameterModel::DemoLoopEvent(), ModelUpdateEvent());

  this->InvokeEvent(ModelUpdateEvent());
}

void SnakeParameterPreviewRenderer
::SetForceToDisplay(SnakeParameterPreviewRenderer::DisplayMode mode)
{
  m_ContextItem->SetForceToDisplay(mode);
}

void SnakeParameterPreviewRenderer::OnUpdate()
{
  if(m_Model && m_Pipeline)
    {
    m_Model->Update();
    m_Pipeline->Update();
    }
}
