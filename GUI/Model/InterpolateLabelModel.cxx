#include "InterpolateLabelModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ScalarImageWrapper.h"

#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryBallStructuringElement.h"

#include "MultiLabelMeshPipeline.h"

void InterpolateLabelModel::SetParentModel(GlobalUIModel *parent)
{
  this->m_Parent = parent;

  m_DrawingLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_InterpolateLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_DrawOverFilterModel->Initialize(parent->GetDriver()->GetColorLabelTable());
}

void InterpolateLabelModel::Interpolate()
{
  // Get the segmentation wrapper
  LabelImageWrapper *liw =
      this->m_Parent->GetDriver()->GetCurrentImageData()->GetSegmentation();

  // We will call C3D for this


}

InterpolateLabelModel::InterpolateLabelModel()
{
  m_InterpolateAllModel = NewSimpleConcreteProperty(false);

  m_DrawingLabelModel = ConcreteColorLabelPropertyModel::New();
  m_InterpolateLabelModel = ConcreteColorLabelPropertyModel::New();
  m_DrawOverFilterModel = ConcreteDrawOverFilterPropertyModel::New();

  m_SmoothingModel = NewRangedConcreteProperty(3.0, 0.0, 20.0, 0.01);
}

