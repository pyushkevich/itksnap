#include "SnakeROIResampleModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"

SnakeROIResampleModel::SnakeROIResampleModel()
{
  // Set up the property models
  m_InputSpacingModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetInputSpacingValueAndRange);

  m_OutputSpacingModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetOutputSpacingValueAndRange, &Self::SetOutputSpacingValue);

  // Create the domain for the drop down boxes
  m_ScaleFactorDomain[1. / 5.] = "1 : 5 (supersample)";
  m_ScaleFactorDomain[1. / 4.] = "1 : 4 (supersample)";
  m_ScaleFactorDomain[1. / 3.] = "1 : 3 (supersample)";
  m_ScaleFactorDomain[1. / 2.] = "1 : 2 (supersample)";
  m_ScaleFactorDomain[1. / 1.] = "1 : 1 (no change)";
  m_ScaleFactorDomain[2.]      = "2 : 1 (subsample)";
  m_ScaleFactorDomain[3.]      = "3 : 1 (subsample)";
  m_ScaleFactorDomain[4.]      = "4 : 1 (subsample)";
  m_ScaleFactorDomain[5.]      = "5 : 1 (subsample)";

  // Create the model for the scale factors
  for(int i = 0; i < 3; i++)
    {
    m_ScaleFactorModel[i] =
        wrapIndexedGetterSetterPairAsProperty(
          this, i,
          &Self::GetIndexedScaleFactorValueAndRange,
          &Self::SetIndexedScaleFactorValue);
    }
}

void SnakeROIResampleModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;

  // Listen to changes in the parent's segmentation ROI settings
  Rebroadcast(
        m_Parent->GetDriver()->GetGlobalState()->GetSegmentationROISettingsModel(),
        ValueChangedEvent(), ModelUpdateEvent());

  // Layer change events too?
  Rebroadcast(m_Parent->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());
}

void SnakeROIResampleModel::ComputeSpacingDomain(NumericValueRange<Vector3d> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  Vector3d spacing = app->GetCurrentImageData()->GetImageSpacing();

  // The reasonable range for the value is not obvious. The largest possible
  // value should be the dimension of the image times the voxel size. The
  // smallest should be something reasonable, like voxel size/10, rounded,
  // since any larger interpolation would be infeasible. The step should probably
  // equal the smallest value
  Vector3ui sz = app->GetCurrentImageData()->GetImageRegion().GetSize();
  Vector3d step = spacing * 0.1;
  for(int i = 0; i < 3; i++)
    step[i] = pow(10, floor(log10(step[i])));

  domain->Set(step, element_product(to_double(sz), spacing), step);
}

bool SnakeROIResampleModel::GetInputSpacingValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    value = app->GetCurrentImageData()->GetImageSpacing();

    if(domain)
      this->ComputeSpacingDomain(domain);

    return true;
    }
  return false;
}

bool SnakeROIResampleModel::GetOutputSpacingValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    SNAPSegmentationROISettings roi = app->GetGlobalState()->GetSegmentationROISettings();
    Vector3d scale = roi.GetVoxelScale();
    Vector3d spacing = app->GetCurrentImageData()->GetImageSpacing();

    value = element_product(scale, spacing);

    if(domain)
      this->ComputeSpacingDomain(domain);

    return true;
    }
  return false;
}

void SnakeROIResampleModel::SetOutputSpacingValue(Vector3d value)
{
  IRISApplication *app = m_Parent->GetDriver();
  SNAPSegmentationROISettings roi = app->GetGlobalState()->GetSegmentationROISettings();
  Vector3d spacing = app->GetCurrentImageData()->GetImageSpacing();
  roi.SetVoxelScale(element_quotient(value,spacing));
  app->GetGlobalState()->SetSegmentationROISettings(roi);
}

bool SnakeROIResampleModel::GetIndexedScaleFactorValueAndRange(
    int index, double &value, ScaleFactorDomain *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    SNAPSegmentationROISettings roi = app->GetGlobalState()->GetSegmentationROISettings();
    Vector3d scale = roi.GetVoxelScale();
    value = scale[index];
    if(domain)
      *domain = m_ScaleFactorDomain;
    std::cout << "Get: Scale[" << index << "] = " << scale[index] << std::endl;
    return true;
    }
  return false;
}

void SnakeROIResampleModel::SetIndexedScaleFactorValue(
    int index, double value)
{
  IRISApplication *app = m_Parent->GetDriver();
  SNAPSegmentationROISettings roi = app->GetGlobalState()->GetSegmentationROISettings();
  Vector3d scale = roi.GetVoxelScale();
  scale[index] = value;
  roi.SetVoxelScale(scale);
  app->GetGlobalState()->SetSegmentationROISettings(roi);
}
