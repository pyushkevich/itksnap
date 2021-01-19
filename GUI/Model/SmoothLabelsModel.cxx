// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkLabelVotingImageFilter.h"

SmoothLabelsModel::SmoothLabelsModel()
{
  // Create a new instance of the model
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();
}

void SmoothLabelsModel::SetParentModel(GlobalUIModel *parent)
{
  m_Parent = parent;
  m_LabelTable = parent->GetDriver()->GetColorLabelTable();

  // Stick the color label information into the domain object
  m_CurrentLabelModel->Initialize(m_LabelTable);
  m_CurrentLabelModel->SetValue(parent->GetDriver()->GetGlobalState()->GetDrawingColorLabel());

  // When label table changed somewhere else, update current model as well
  Rebroadcast(m_LabelTable,SegmentationChangeEvent(), ModelUpdateEvent());
}

void SmoothLabelsModel::UpdateOnShow()
{

}

void SmoothLabelsModel::Smooth(std::vector<LabelType> &labelsToSmooth, std::vector<double> &sigma, SigmaUnit unit)
{
  std::cout << "Labels to Smooth: " << endl;
  for (auto cit = labelsToSmooth.cbegin(); cit != labelsToSmooth.cend(); ++cit)
    {
      std::cout << *cit << endl;
    }

  std::cout << "Sigma Array" << endl;
  for (auto it = sigma.begin(); it != sigma.end(); ++it)
    {
      std::cout << std::to_string(*it) << " ";
    }
  std::cout << std::endl;

  // Get the segmentaton wrapper
  LabelImageWrapper *liw = m_Parent->GetDriver()->GetSelectedSegmentationLayer();

  // Process sigma user input
  std::vector<double> sigmaInput = sigma; // deep copy

  // ITK Smoothing filter uses sigmas in millimeters.
  // Convert vox sigma into millimeters
  if (unit == SigmaUnit::vox)
    {
      std::cout << "Unit: vox " << std::endl;
      typedef LabelImageWrapper::ImageType::SpacingType SpacingType;
      SpacingType spacing = liw->GetImage()->GetSpacing();
      for (int i = 0; i < 3; ++i)
        sigmaInput[i] *= spacing[i];
    }

  std::cout << "Converted Sigma Array: " << endl;
  for (auto it = sigmaInput.begin(); it != sigmaInput.end(); ++it)
    std::cout << std::to_string(*it) << " ";

  std::cout << endl;

  // Get labels to smooth
  LabelType target = labelsToSmooth[0];

  // Apply itk smoothing filter
  typedef GenericImageData::LabelImageType LabelImageType;
  //typedef itk::Image<int> ImageType;
  typedef RLEImage<unsigned short> ImageType;
  typedef itk::SmoothingRecursiveGaussianImageFilter<ImageType, ImageType> FilterType;
  FilterType::Pointer fltSmooth = FilterType::New();

  /*
  typedef LabelImageWrapper::ImageType ImageType;
  typedef itk::SmoothingRecursiveGaussianImageFilter<ImageType, ImageType> FilterType;
  FilterType::Pointer fltSmooth = FilterType::New();
  FilterType::SigmaArrayType sigArr;

  typedef itk::LabelVotingImageFilter<ImageType, ImageType> LabelFilterType;
  LabelFilterType::Pointer fltVote = LabelFilterType::New();


  // -- Pass in user specified sigma values
  for (int i = 0; i < 3; ++i)
    sigArr[i] = sigmaInput[i];
  */


  // -- Threshold out background (1) and other (0)
  // -- Use the result to initialize the voting

  // -- for each label to smooth:
  // ---- binarize the label: current label set to 1 and rest to 0
  // ---- smooth the binarized the label image
  // ---- vote against the voted image, and generate a new voted image


  // apply itk voting filter
}
