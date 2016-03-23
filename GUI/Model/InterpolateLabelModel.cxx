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

  m_DrawingLabel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_InterpolateLabel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_DrawOverFilter->Initialize(parent->GetDriver()->GetColorLabelTable());
}

void InterpolateLabelModel::Interpolate()
{
  // Get the segmentation wrapper
  LabelImageWrapper *liw =
      this->m_Parent->GetDriver()->GetCurrentImageData()->GetSegmentation();

  // Use the multi-label mesh pipeline to generate the per-label bounding boxes
  SmartPtr<MultiLabelMeshPipeline> mpipe = MultiLabelMeshPipeline::New();
  mpipe->SetImage(liw);
  mpipe->ComputeBoundingBoxes();

  // Get the bounding boxes and related information for all the labels
  MultiLabelMeshPipeline::MeshInfoMap mim = mpipe->GetMeshInfo();

  // For now, just deal with a single label
  LabelType l_interp = m_InterpolateLabel->GetValue();
  if(mim.find(l_interp) == mim.end())
    {
    // TODO: complain to the user
    return;
    }

  // Point to the mesh info
  const MultiLabelMeshPipeline::MeshInfo &mi = mim.find(l_interp)->second;

  // Convert bounding box to a region




  // Start by finding the foreground region


}

InterpolateLabelModel::InterpolateLabelModel()
{
  m_InterpolateAll = NewSimpleConcreteProperty(false);

  m_DrawingLabel = ConcreteColorLabelPropertyModel::New();
  m_InterpolateLabel = ConcreteColorLabelPropertyModel::New();
  m_DrawOverFilter = ConcreteDrawOverFilterPropertyModel::New();

  m_Smoothing = new NewRangedConcreteProperty(3.0, 0.0, 20.0, 0.01);
}

InterpolateLabelModel::~InterpolateLabelModel()
{
}

