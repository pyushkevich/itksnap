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
template <class TInputImage1, class TInputImage2, class TOutputImage>
class BinaryIntensityVotingFunctor
{
public:
  typedef BinaryIntensityVotingFunctor Self;
  typedef typename TInputImage1::PixelType InputPixelType1;
  typedef typename TInputImage2::PixelType InputPixelType2;
  typedef typename TOutputImage::PixelType OutputPixelType;

  BinaryIntensityVotingFunctor() = default;
  ~BinaryIntensityVotingFunctor() = default;

  OutputPixelType operator() (const InputPixelType1 &crntPixel, const InputPixelType2 &maxPixel) const
  {
    return crntPixel > maxPixel ? crntPixel : maxPixel;
  }


};

template <class TInputImage1, class TInputImage2, class TOutputImage>
class BinaryLabelVotingFunctor
{
public:
  typedef BinaryLabelVotingFunctor Self;
  typedef typename TInputImage1::PixelType InputPixelType1;
  typedef typename TInputImage2::PixelType InputPixelType2;
  typedef typename TOutputImage::PixelType OutputPixelType;

  BinaryLabelVotingFunctor(OutputPixelType crntLabel)
    : m_crntLabel(crntLabel) {};

  // default constructor is needed for declaration of the functor filter
  BinaryLabelVotingFunctor() {};
  ~BinaryLabelVotingFunctor() = default;

  OutputPixelType operator() (const InputPixelType1 &crntPixel, const InputPixelType2 &maxPixel) const
  {
    return crntPixel > maxPixel ? m_crntLabel : 0;
  }

  bool operator != (const Self &other)
  {
    return other.m_crntLabel != m_crntLabel;
  }

private:
  OutputPixelType m_crntLabel;
};

template <class TInputImage1, class TInputImage2,class TOutputImage>
class BinaryLabelDeterminatingFunctor
{
public:
  typedef BinaryLabelDeterminatingFunctor Self;
  typedef typename TInputImage1::PixelType InputPixelType1;
  typedef typename TInputImage2::PixelType InputPixelType2;
  typedef typename TOutputImage::PixelType OutputPixelType;

  BinaryLabelDeterminatingFunctor() = default;
  ~BinaryLabelDeterminatingFunctor() = default;

  OutputPixelType operator() (const InputPixelType1 &crntLabelResult, const InputPixelType2 &globalLabelResult) const
  {
    return crntLabelResult == 0 ? globalLabelResult : crntLabelResult;
  }
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
  typedef itk::BinaryThresholdImageFilter<LabelImageType, VotingImageType> BlankVotingImageGenerator;
  typedef itk::BinaryThresholdImageFilter<LabelImageType, LabelImageType> BlankLabelImageGenerator;
  typedef BinaryIntensityVotingFunctor<VotingImageType, VotingImageType, VotingImageType> IntensityVotingFunctor;
  typedef itk::BinaryFunctorImageFilter
      <VotingImageType, VotingImageType, VotingImageType, IntensityVotingFunctor> IntensityVoter;
  typedef BinaryLabelVotingFunctor<VotingImageType, VotingImageType, LabelImageType> LabelVotingFunctor;
  typedef itk::BinaryFunctorImageFilter
      <VotingImageType, VotingImageType, LabelImageType, LabelVotingFunctor> LabelVoter;
  typedef BinaryLabelDeterminatingFunctor<LabelImageType, LabelImageType, LabelImageType> LabelDeterminatingFunctor;
  typedef itk::BinaryFunctorImageFilter
      <LabelImageType, LabelImageType, LabelImageType, LabelDeterminatingFunctor> LabelDeterminator;
  typedef itk::SmoothingRecursiveGaussianImageFilter<BinarizedImageType, VotingImageType> SmoothFilter;

  // Generate a blank (all zero) image for counting maximum intensities
  BlankVotingImageGenerator::Pointer blankVotingImageGen = BlankVotingImageGenerator::New();
  blankVotingImageGen->SetInput(liw->GetImage());
  blankVotingImageGen->SetLowerThreshold(0);
  blankVotingImageGen->SetUpperThreshold(0);
  blankVotingImageGen->SetInsideValue(0.0);
  blankVotingImageGen->SetOutsideValue(0.0);
  blankVotingImageGen->Update();
  VotingImageType::Pointer maxIntensity = blankVotingImageGen->GetOutput();
  // Generate a blank (all zero) image for recording winning labels
  BlankLabelImageGenerator::Pointer blankLabelImageGen = BlankLabelImageGenerator::New();
  blankLabelImageGen->SetInput(liw->GetImage());
  blankLabelImageGen->SetLowerThreshold(0);
  blankLabelImageGen->SetUpperThreshold(0);
  blankLabelImageGen->SetInsideValue(0);
  blankLabelImageGen->SetOutsideValue(0);
  blankLabelImageGen->Update();
  LabelImageType::Pointer winningLabels = blankLabelImageGen->GetOutput();

  // Declare voting filters
  IntensityVoter::Pointer intensityVoter = IntensityVoter::New();
  LabelVoter::Pointer labelVoter = LabelVoter::New();
  LabelDeterminator::Pointer labelDeterminator = LabelDeterminator::New();

  // Push background label (0) to the array
  labelArr.push_back(0);


  // Iterate labels; Smooth and Vote
  for (auto it = labelArr.begin(); it != labelArr.end(); ++it)
    {
      std::cout << "Processing: " << *it << endl;

      // Binarize the Image
      ThresholdFilter::Pointer fltThreshold = ThresholdFilter::New();

      // -- set current seg image layer as input
      fltThreshold->SetInput(liw->GetImage());
      // -- set current target label (*it) to 1, others to 0
      fltThreshold->SetLowerThreshold(*it);
      fltThreshold->SetUpperThreshold(*it);
      fltThreshold->SetInsideValue(1.0);
      fltThreshold->SetOutsideValue(0.0);
      fltThreshold->Update();

      // Smooth the binarized image
      SmoothFilter::Pointer fltSmooth = SmoothFilter::New();
      fltSmooth->SetInput(fltThreshold->GetOutput());

      // -- Set sigma array
      SmoothFilter::SigmaArrayType sigmaArr;
      for (int i = 0; i < 3; ++i)
        sigmaArr[i] = sigmaInput[i];
      fltSmooth->SetSigmaArray(sigmaArr);

      // -- Smooth
      fltSmooth->Update();
      VotingImageType::Pointer crntLabel = fltSmooth->GetOutput();

      // Vote to determine the label

      /* Intensity Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest instensity. If current intensity is greater than previous high,
       * it will replace previous high in the output image. Otherwise, previous high will be
       * preserved in the output image.
       */
      intensityVoter->SetInput1(crntLabel);
      intensityVoter->SetInput2(maxIntensity);
      intensityVoter->Update();
      // Temporarily save the new max intensity, since maxIntensity needs to be used again
      VotingImageType::Pointer newMaxIntensity = intensityVoter->GetOutput();

      /* Label Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest intensity. If current intensity is greater than previous high,
       * current label will be written to the output image. Otherwise, 0 will be written.
       */
      LabelVotingFunctor lvf(*it); // pass current label into the functor
      labelVoter->SetFunctor(lvf);
      labelVoter->SetInput1(crntLabel);
      labelVoter->SetInput2(maxIntensity);
      labelVoter->Update();
      LabelImageType::Pointer labelVotingResult = labelVoter->GetOutput();

      /* Label Determination:
       * Pixel-wise iteration on current label voting result and the global label voting result.
       * If current result is non-zero, replace global result value with the current result value.
       */
      labelDeterminator->SetInput1(labelVotingResult);
      labelDeterminator->SetInput2(winningLabels);
      labelDeterminator->Update();
      winningLabels = labelDeterminator->GetOutput();

      // update maxIntensity with the current max
      maxIntensity = newMaxIntensity;
    }




  // apply itk voting filter
}
