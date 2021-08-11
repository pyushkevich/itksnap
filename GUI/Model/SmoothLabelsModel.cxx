// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageDuplicator.h"
#include "itkBinaryFunctorImageFilter.h"
#include "SegmentationUpdateIterator.h"
#include "SegmentationStatistics.h"


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

// Debugging
/*
template<class ImageType>
class DebuggingFileWriter
{
public:
  typedef itk::ImageFileWriter<ImageType> ImageWriterType;
  DebuggingFileWriter() : m_Writer(ImageWriterType::New()) {};
  void WriteToFile (std::string &fileName, typename ImageType::Pointer data)
  {

    m_Writer->SetFileName(fileName);
    m_Writer->SetInput(data);
    try
    {
      m_Writer->Update();
    } catch (itk::ExceptionObject &error)
    {
      std::cerr << "Error: " << error << std::endl;
    }
  }
private:
  typename ImageWriterType::Pointer m_Writer;
};
*/

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

template<typename TImage>
void SmoothLabelsModel::DeepCopy(typename TImage::Pointer input, typename TImage::Pointer output)
{
  output->SetRegions(input->GetLargestPossibleRegion());
  output->Allocate();

  itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
  itk::ImageRegionIterator<TImage>      outputIterator(output, output->GetLargestPossibleRegion());

  while (!inputIterator.IsAtEnd())
  {
    outputIterator.Set(inputIterator.Get());
    ++inputIterator;
    ++outputIterator;
  }
}

void SmoothLabelsModel::UpdateOnShow()
{

}

GlobalUIModel* SmoothLabelsModel::GetParent() const
{
  return this->m_Parent;
}

#include "ConvertAPI.h"


void
SmoothLabelsModel
::ApplyC3DSmoothing(LabelImageWrapper *liw, std::vector<double> sigma
                    , SigmaUnit sigmaUnit, std::unordered_set<LabelType> labelsToSmooth)
{
  typedef ConvertAPI<double, 3> ConvertAPIType;
  typedef LabelImageWrapper::ImageType LabelImageType;
  typedef typename ConvertAPIType::ImageType C3DImageType;


  C3DImageType::Pointer c3dInputImage = C3DImageType::New();

  std::cout << "[apply c3d smoothing] start preprocessing" << std::endl;

  c3dInputImage->SetRegions(liw->GetImage()->GetLargestPossibleRegion());
  c3dInputImage->Allocate();

  std::cout << "[apply c3d smoothing] image allocated" << std::endl;

  itk::ImageRegionIterator<C3DImageType>
      it_c3d(c3dInputImage, c3dInputImage->GetLargestPossibleRegion());

  std::cout << "[apply c3d smoothing] it_c3d created" << std::endl;

  itk::ImageRegionConstIterator<LabelImageType>
      it_label(liw->GetImage(), liw->GetBufferedRegion());

  std::cout << "[apply c3d smoothing] start converting liw to c3d image" << std::endl;

  while (!it_label.IsAtEnd())
    {
      it_c3d.Set(it_label.Get());
      ++it_c3d;
      ++it_label;
    }

  std::cout << "[apply c3d smoothing] start api call" << std::endl;

  // Build label string
  std::string labelString;
  for (auto cit = labelsToSmooth.cbegin(); cit != labelsToSmooth.cend(); ++cit)
    {
      labelString += std::to_string(*cit) + ' ';
    }

  ConvertAPIType c3d;

  // Add Input Image to the stack
  c3d.AddImage("imgin", c3dInputImage);

  // Redirect c3d standard outputs
  c3d.RedirectOutput(std::cout, std::cerr);

  c3d.Execute(
        "-verbose -clear -push imgin -smooth-multilabel %fx%fx%f%s \"%s\" -as imgout",
        sigma[0], sigma[1], sigma[2], SigmaUnitStr[sigmaUnit], labelString.c_str()
  );

  C3DImageType::Pointer c3dOutputImage = c3d.GetImage("imgout");

  // Apply labels back to segmentation image
  SegmentationUpdateIterator it_update(liw, liw->GetBufferedRegion()
                                       , m_Parent->GetGlobalState()->GetDrawingColorLabel()
                                       , m_Parent->GetGlobalState()->GetDrawOverFilter());
  itk::ImageRegionConstIterator<C3DImageType>
      it_src(c3dOutputImage, c3dOutputImage->GetBufferedRegion());

  std::cout << "Updating GUI..." << endl;
  for (; !it_update.IsAtEnd(); ++it_update, ++it_src)
      it_update.PaintLabel(it_src.Get());

  // Finalize update and create an undo point
  it_update.Finalize("Smooth Labels");

  // Fire event to inform GUI that segmentation has changed
  this->m_Parent->GetDriver()->InvokeEvent(SegmentationChangeEvent());
  std::cout << "Selected labels have been smoothed!" << std::endl;
}

void
SmoothLabelsModel
::Smooth(std::unordered_set<LabelType> &labelsToSmooth,
         std::vector<double> &sigma,
         SigmaUnit unit,
         bool SmoothAllFrames)
{
  // Always smooth background
  if (!labelsToSmooth.count(0))
    labelsToSmooth.insert(0);

  // --Debug
  {
    std::cout << "Labels to Smooth: " << endl;
    for (auto cit = labelsToSmooth.cbegin(); cit != labelsToSmooth.cend(); ++cit)
        std::cout << *cit << endl;
    std::cout << "Sigma Array" << endl;
    for (auto it = sigma.begin(); it != sigma.end(); ++it)
        std::cout << std::to_string(*it) << " ";
    std::cout << "SmoothAllFrames: " << SmoothAllFrames << std::endl;
    std::cout << std::endl;
  }

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


  unsigned int nT = liw->GetNumberOfTimePoints();
  std::cout << "Number of Frames: " << nT << std::endl;

  // For 3D Image, It will always be 0;
  // For 4D Image, Smooth All will start from 0, otherwise current time point
  unsigned int crntFrame = SmoothAllFrames ? 0 : liw->GetTimePointIndex();

  // For 3D Image, It will always be 1
  // For 4D Image, Smooth All will end with last frame, otherwise next time point
  const unsigned int frameEnd = SmoothAllFrames ? nT : crntFrame + 1;

  SegmentationStatistics statsCalculator;
  typedef SegmentationStatistics::EntryMap EntryMap;
  const EntryMap &stats = statsCalculator.GetStats();

  // Set of valid labels for current frame
  std::vector<LabelType> allLabels;


  // Apply the c3d multilabel smoothing api
  this->ApplyC3DSmoothing(liw, sigmaInput, unit, labelsToSmooth);

  // Change label image to current frame
  liw->SetTimePointIndex(m_Parent->GetDriver()->GetCursorTimePoint());
  liw->Modified();

  // --Debug
  if (SmoothAllFrames)
    std::cout << "All Frames have been smoothed" << std::endl;
  else
    std::cout << "Current frame has been smoothed" << std::endl;
}
