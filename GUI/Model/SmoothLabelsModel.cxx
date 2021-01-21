// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkLabelVotingImageFilter.h"
#include "itkImageDuplicator.h"
#include "itkBinaryFunctorImageFilter.h"

class BinaryIntensityVotingFunctor
{

};

class BinaryLabelVotingFunctor
{

};

class BinaryLabelDeterminatingFunctor
{

};

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
  std::vector<LabelType> labelArr {0}; // always include background
  for (auto it = labelsToSmooth.begin(); it != labelsToSmooth.end(); ++it)
    labelArr.push_back(*it);

  // Type definitions
  typedef GenericImageData::LabelImageType LabelImageType;
  // -- Output of Binarization
  typedef itk::Image<unsigned short, 3> BinarizedImageType;
  // -- Output of Smoothing, Input of Voting
  typedef itk::Image<double, 3> VotingImageType;

  typedef itk::BinaryThresholdImageFilter<LabelImageType, BinarizedImageType> ThresholdFilter;
  typedef itk::BinaryThresholdImageFilter<LabelImageType, VotingImageType> BackgroundExtractor;
  typedef itk::BinaryFunctorImageFilter
      <VotingImageType, VotingImageType, VotingImageType, BinaryIntensityVotingFunctor> IntensityVoter;
  typedef itk::BinaryFunctorImageFilter
      <VotingImageType, VotingImageType, LabelImageType, BinaryLabelVotingFunctor> LabelVoter;
  typedef itk::BinaryFunctorImageFilter
      <LabelImageType, LabelImageType, LabelImageType, BinaryLabelDeterminatingFunctor> LabelDeterminator;
  typedef itk::SmoothingRecursiveGaussianImageFilter<BinarizedImageType, VotingImageType> SmoothFilter;
  typedef itk::ImageDuplicator<VotingImageType> Duplicator;

  // Duplicate a background image for counting votes
  // -- Use thresholding and duplicator to avoid creating and initializing new image
  BackgroundExtractor::Pointer bgExtractor = BackgroundExtractor::New();
  bgExtractor->SetInput(liw->GetImage());
  bgExtractor->SetLowerThreshold(0);
  bgExtractor->SetUpperThreshold(0);
  bgExtractor->SetInsideValue(0.0);
  bgExtractor->SetOutsideValue(0.0);
  Duplicator::Pointer duplicator = Duplicator::New();
  duplicator->SetInputImage(bgExtractor->GetOutput());
  VotingImageType::Pointer winningLabels = duplicator->GetOutput();

  // Iterate labels; Smooth and Vote
  for (auto it = labelArr.begin(); it != labelArr.end(); ++it)
    {
      // Binarize the Image
      ThresholdFilter::Pointer fltThreshold = ThresholdFilter::New();

      // -- set current seg image layer as input
      fltThreshold->SetInput(liw->GetImage());
      // -- set current target label (*it) to 1, others to 0
      fltThreshold->SetLowerThreshold(*it);
      fltThreshold->SetUpperThreshold(*it);
      fltThreshold->SetInsideValue(1.0);
      fltThreshold->SetOutsideValue(0.0);

      // Smooth the binarized image
      SmoothFilter::Pointer fltSmooth = SmoothFilter::New();
      fltSmooth->SetInput(fltThreshold->GetOutput());

      // -- Set sigma array
      SmoothFilter::SigmaArrayType sigmaArr;
      for (int i = 0; i < 3; ++i)
        sigmaArr[i] = sigmaInput[i];

      // -- Smooth

      // Vote to determine the label

      /* Intensity Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest instensity. If current intensity is greater than previous high,
       * it will replace previous high in the output image. Otherwise, previous high will be
       * preserved in the output image.
       */

      /* Label Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest intensity. If current intensity is greater than previous high,
       * current label will be written to the output image. Otherwise, 0 will be written.
       */

      /* Label Determination:
       * Pixel-wise iteration on current label voting result and the global label voting result.
       * If current result is non-zero, replace global result value with the current result value.
       */

    }




  // apply itk voting filter
}
