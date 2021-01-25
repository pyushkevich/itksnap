// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageDuplicator.h"
#include "itkBinaryFunctorImageFilter.h"
#include "SegmentationUpdateIterator.h"

#include "itkImageFileWriter.h"

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

void SmoothLabelsModel::Smooth(std::unordered_set<LabelType> &labelsToSmooth, std::vector<double> &sigma, SigmaUnit unit)
{
  // Always smooth background
  if (!labelsToSmooth.count(0))
    labelsToSmooth.insert(0);

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

  // Get all valid labels
  std::vector<LabelType> allLabels;
  ColorLabelTable::ValidLabelMap validLabelMap = m_LabelTable->GetValidLabels();

  for(auto cit = validLabelMap.cbegin(); cit != validLabelMap.cend(); ++cit)
    {
      allLabels.push_back(cit->first);
    }

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

  // Type definitions
  typedef GenericImageData::LabelImageType LabelImageType;
  // -- Output of Smoothing, Input of Voting
  typedef itk::Image<double, 3> VotingImageType;

  typedef itk::BinaryThresholdImageFilter<LabelImageType, VotingImageType> ThresholdFilter;
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
  typedef itk::SmoothingRecursiveGaussianImageFilter<VotingImageType, VotingImageType> SmoothFilter;

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

  // Debug
  using VotingImageWriter = itk::ImageFileWriter<VotingImageType>;
  VotingImageWriter::Pointer votingImageWriter = VotingImageWriter::New();

  // Iterate labels; Smooth and Vote
  for (auto cit = allLabels.cbegin(); cit != allLabels.cend(); ++cit)
    {
      std::cout << "Processing: " << *cit << endl;

      // Binarize the Image
      ThresholdFilter::Pointer fltThreshold = ThresholdFilter::New();

      // -- set current seg image layer as input
      fltThreshold->SetInput(liw->GetImage());
      // -- set current target label (*cit) to 1, others to 0
      fltThreshold->SetLowerThreshold(*cit);
      fltThreshold->SetUpperThreshold(*cit);
      fltThreshold->SetInsideValue(1.0);
      fltThreshold->SetOutsideValue(0.0);
      fltThreshold->Update();

      // Smooth selected labels. For unselected labels, keep intensity as 1
      bool isLabelSelected = labelsToSmooth.count(*cit);
      // -- keep intensity for unselected label
      VotingImageType::Pointer crntLabelBinarized = fltThreshold->GetOutput();

      votingImageWriter->SetFileName("BinarizedResult_Label_" + std::to_string(*cit) + ".nii.gz");
      votingImageWriter->SetInput(crntLabelBinarized);
      try
      {
        votingImageWriter->Update();
      } catch (itk::ExceptionObject &error)
      {
        std::cerr << "Error: " << error << std::endl;
      }

      if (isLabelSelected)
        {
          std::cout << "Smoothing..." << endl;
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

          crntLabelBinarized = fltSmooth->GetOutput();
          std::cout << "Smoothing Completed." << endl;
        }

      votingImageWriter->SetFileName("IntensityVotingResult_Label_" + std::to_string(*cit) + ".nii.gz");
      votingImageWriter->SetInput(crntLabelBinarized);
      try
      {
        votingImageWriter->Update();
      } catch (itk::ExceptionObject &error)
      {
        std::cerr << "Error: " << error << std::endl;
      }
      // Vote to determine the label

      /* Intensity Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest instensity. If current intensity is greater than previous high,
       * it will replace previous high in the output image. Otherwise, previous high will be
       * preserved in the output image.
       */

      intensityVoter->SetInput1(crntLabelBinarized);
      intensityVoter->SetInput2(maxIntensity);
      intensityVoter->Update();

      // Temporarily save the new max intensity, since maxIntensity needs to be used again
      VotingImageType::Pointer newMaxIntensity = intensityVoter->GetOutput();

      votingImageWriter->SetFileName("LabelVotingResult_Label_" + std::to_string(*cit) + ".nii.gz");
      votingImageWriter->SetInput(newMaxIntensity);
      try
      {
        votingImageWriter->Update();
      } catch (itk::ExceptionObject &error)
      {
        std::cerr << "Error: " << error << std::endl;
      }

      std::cout << "Intensity Voting Completed" << endl;

      /* Label Voting:
       * A pixel-wise intensity comparison between current label smoothing result
       * and the previous highest intensity. If current intensity is greater than previous high,
       * current label will be written to the output image. Otherwise, 0 will be written.
       */
      LabelVotingFunctor lvf(*cit); // pass current label into the functor
      labelVoter->SetFunctor(lvf);
      labelVoter->SetInput1(crntLabelBinarized);
      labelVoter->SetInput2(maxIntensity);
      labelVoter->Update();
      LabelImageType::Pointer labelVotingResult = labelVoter->GetOutput();

      std::cout << "Label Voting Completed" << endl;

      /* Label Determination:
       * Pixel-wise iteration on current label voting result and the global label voting result.
       * If current result is non-zero, replace global result value with the current result value.
       */
      labelDeterminator->SetInput1(labelVotingResult);
      labelDeterminator->SetInput2(winningLabels);
      labelDeterminator->Update();
      winningLabels = labelDeterminator->GetOutput();

      std::cout << "Label Determined" << endl;

      // update maxIntensity with the current max
      maxIntensity = newMaxIntensity;

    }

  // Apply labels back to segmentation image
  SegmentationUpdateIterator it_update(liw, liw->GetBufferedRegion()
                                       , m_Parent->GetGlobalState()->GetDrawingColorLabel()
                                       , m_Parent->GetGlobalState()->GetDrawOverFilter());
  itk::ImageRegionConstIterator<GenericImageData::LabelImageType>
      it_src(winningLabels, winningLabels->GetBufferedRegion());

  std::cout << "Updating GUI..." << endl;
  for (; !it_update.IsAtEnd(); ++it_update, ++it_src)
    {
      it_update.PaintLabel(it_src.Get());
    }

  // Finalize update and create an undo point
  it_update.Finalize("Smooth Labels");

  // Fire event to inform GUI that segmentation has changed
  this->m_Parent->GetDriver()->InvokeEvent(SegmentationChangeEvent());
  std::cout << "Selected labels has been smoothed!" << endl;

}
