#include "SnakeROIResampleModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "SNAPSegmentationROISettings.h"

SnakeROIResampleModel::SnakeROIResampleModel()
{
  // Set up the property models
  for(int i = 0; i < 3; i++)
    {
    m_InputSpacingModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetInputSpacingValueAndRange);

    m_InputDimensionsModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetInputDimensionsValueAndRange);

    m_OutputSpacingModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetOutputSpacingValueAndRange, &Self::SetOutputSpacingValue);

    m_OutputDimensionsModel[i] = wrapIndexedGetterSetterPairAsProperty(
          this, i, &Self::GetOutputDimensionsValueAndRange, &Self::SetOutputDimensionsValue);
    }

  // Aspect ratio
  m_FixedAspectRatioModel = NewSimpleConcreteProperty(false);

  // Create the interpolation model
  m_InterpolationModeModel = ConcreteInterpolationModeModel::New();
}

void SnakeROIResampleModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
  m_ROISettingsModel =
      m_Parent->GetDriver()->GetGlobalState()->GetSegmentationROISettingsModel();

  // Listen to changes in the parent's segmentation ROI settings
  Rebroadcast(m_ROISettingsModel, ValueChangedEvent(), ModelUpdateEvent());

  // Layer change events too
  Rebroadcast(m_Parent->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());
}

void SnakeROIResampleModel::Reset()
{
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
  m_ResampleDimensions = roi.GetROI().GetSize();
  m_InterpolationModeModel->SetValue(SNAPSegmentationROISettings::TRILINEAR);
  InvokeEvent(ModelUpdateEvent());
}

void SnakeROIResampleModel::Accept()
{
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
  roi.SetResampleDimensions(m_ResampleDimensions);
  roi.SetInterpolationMethod(m_InterpolationModeModel->GetValue());
  m_ROISettingsModel->SetValue(roi);
}

void SnakeROIResampleModel::ApplyPreset(SnakeROIResampleModel::ResamplePreset preset)
{
  // Get the current spacing
  IRISApplication *app = m_Parent->GetDriver();
  Vector3d in_spacing = app->GetCurrentImageData()->GetImageSpacing();
  Vector3d out_spacing;

  if(preset == SUPER_2)
    {
    out_spacing = in_spacing / 2.0;
    }
  else if(preset == SUB_2)
    {
    out_spacing = in_spacing * 2.0;
    }
  if(preset == SUPER_ISO)
    {
    double mindim = in_spacing.min_value();
    out_spacing.fill(mindim);
    this->SetFixedAspectRatio(false);
    }
  else if(preset == SUB_ISO)
    {
    double maxdim = in_spacing.max_value();
    out_spacing.fill(maxdim);
    this->SetFixedAspectRatio(false);
    }

  for(int i = 0; i < 3; i++)
    m_OutputSpacingModel[i]->SetValue(out_spacing[i]);

}

void SnakeROIResampleModel::ComputeCachedDomains()
{
  IRISApplication *app = m_Parent->GetDriver();
  Vector3d spacing = app->GetCurrentImageData()->GetImageSpacing();
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();

  // The reasonable range for the value is not obvious. The largest possible
  // value should be the dimension of the image times the voxel size. The
  // smallest should be something reasonable, like voxel size/10, rounded,
  // since any larger interpolation would be infeasible. The step should probably
  // equal the smallest value
  Vector3ui sz = roi.GetROI().GetSize();
  for(int i = 0; i < 3; i++)
    {
    double step = pow(10, floor(log10(spacing[i] / 10.0)));
    m_SpacingDomain[i].Set(step, sz[i] * spacing[i], step);
    m_DimensionsDomain[i].Set(1, sz[i]*10, 1);
    }

  // TODO: higher order interpolation methods are currently unsupported for
  // vector images. This is a problem and should be fixed

  // Set up the interpolation mode map
  m_InterpolationModeDomain[SNAPSegmentationROISettings::NEAREST_NEIGHBOR] =
      "Nearest neighbor (fast)";
  m_InterpolationModeDomain[SNAPSegmentationROISettings::TRILINEAR] =
      "Linear interpolation (better quality)";
  // m_InterpolationModeDomain[SNAPSegmentationROISettings::TRICUBIC] =
  //     "Cubic interpolation (high quality)";
  // m_InterpolationModeDomain[SNAPSegmentationROISettings::SINC_WINDOW_05] =
  //     "Windowed sinc interpolation (best quality)";

  m_InterpolationModeModel->SetDomain(m_InterpolationModeDomain);

}

void SnakeROIResampleModel::EnforceAspectRatio(int source_idx)
{
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();

  double aspect = m_ResampleDimensions[source_idx] * 1.0 / roi.GetROI().GetSize()[source_idx];

  for(int i = 0; i < 3; i++)
    {
    if(i != source_idx)
      {
      unsigned int new_dim =
          itk::Math::Round<double>(aspect * roi.GetROI().GetSize()[i]);
      m_ResampleDimensions[i] = new_dim >= 1 ? new_dim : 1;
      }
    }
}

void SnakeROIResampleModel::OnUpdate()
{
  if(this->m_EventBucket->HasEvent(LayerChangeEvent()) ||
     this->m_EventBucket->HasEvent(ValueChangedEvent(), m_ROISettingsModel))
    {
    if(m_Parent->GetDriver()->IsMainImageLoaded())
      {
      ComputeCachedDomains();
      SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
      m_ResampleDimensions = roi.GetResampleDimensions();
      m_InterpolationModeModel->SetValue(roi.GetInterpolationMethod());
      }
    }
}

bool SnakeROIResampleModel::GetInputSpacingValueAndRange(int index,
    double &value, NumericValueRange<double> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    value = app->GetCurrentImageData()->GetImageSpacing()[index];

    if(domain)
      *domain = m_SpacingDomain[index];

    return true;
    }
  return false;
}

bool SnakeROIResampleModel
::GetInputDimensionsValueAndRange(
    int index, unsigned int &value, NumericValueRange<unsigned int> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
    value = roi.GetROI().GetSize()[index];
    return true;
    }

  return false;
}

bool SnakeROIResampleModel::GetOutputSpacingValueAndRange(
    int index, double &value, NumericValueRange<double> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->IsMainImageLoaded())
    {
    Vector3d inSpacing = app->GetCurrentImageData()->GetImageSpacing();
    SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
    value = roi.GetROI().GetSize()[index] * inSpacing[index] / m_ResampleDimensions[index];

    if(domain)
      *domain = m_SpacingDomain[index];

    return true;
    }
  return false;
}

void SnakeROIResampleModel::SetOutputSpacingValue(int index, double value)
{
  IRISApplication *app = m_Parent->GetDriver();
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
  Vector3d inSpacing = app->GetCurrentImageData()->GetImageSpacing();

  // Calculate the dimensions
  m_ResampleDimensions[index] = itk::Math::Round<double>(
        roi.GetROI().GetSize()[index] * inSpacing[index] / value);

  if(GetFixedAspectRatio())
    EnforceAspectRatio(index);

  InvokeEvent(ModelUpdateEvent());
}

bool SnakeROIResampleModel
::GetOutputDimensionsValueAndRange(int index, unsigned int &value, NumericValueRange<unsigned int> *domain)
{
  IRISApplication *app = m_Parent->GetDriver();
  SNAPSegmentationROISettings roi = m_ROISettingsModel->GetValue();
  if(app->IsMainImageLoaded())
    {
    value = m_ResampleDimensions[index];
    if(domain)
      {
      *domain = m_DimensionsDomain[index];
      }
    return true;
    }

  return false;
}

void SnakeROIResampleModel
::SetOutputDimensionsValue(int index, unsigned int value)
{
  m_ResampleDimensions[index] = value;

  if(GetFixedAspectRatio())
    EnforceAspectRatio(index);

  InvokeEvent(ModelUpdateEvent());
}


