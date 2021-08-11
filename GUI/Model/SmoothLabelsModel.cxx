// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SegmentationUpdateIterator.h"
#include "SegmentationStatistics.h"
#include "ConvertAPI.h"


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

GlobalUIModel* SmoothLabelsModel::GetParent() const
{
  return this->m_Parent;
}

void
SmoothLabelsModel
::ApplyC3DSmoothing(LabelImageWrapper *liw, std::vector<double> sigma
                    , SigmaUnit sigmaUnit, std::unordered_set<LabelType> labelsToSmooth)
{
  typedef ConvertAPI<double, 3> ConvertAPIType;
  typedef LabelImageWrapper::ImageType LabelImageType;
  typedef typename ConvertAPIType::ImageType C3DImageType;

  // Convert LabelImageType to C3DImageType
  C3DImageType::Pointer c3dInputImage = C3DImageType::New();
  c3dInputImage->SetRegions(liw->GetImage()->GetLargestPossibleRegion());
  c3dInputImage->Allocate();

  itk::ImageRegionIterator<C3DImageType>
      it_c3d(c3dInputImage, c3dInputImage->GetLargestPossibleRegion());

  itk::ImageRegionConstIterator<LabelImageType>
      it_label(liw->GetImage(), liw->GetBufferedRegion());

  // Deep copy pixels
  while (!it_label.IsAtEnd())
    {
      it_c3d.Set(it_label.Get());
      ++it_c3d;
      ++it_label;
    }

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

  // Execute API call
  c3d.Execute(
        "-clear -push imgin -smooth-multilabel %fx%fx%f%s \"%s\" -as imgout",
        sigma[0], sigma[1], sigma[2], SigmaUnitStr[sigmaUnit], labelString.c_str()
  );

  C3DImageType::Pointer c3dOutputImage = c3d.GetImage("imgout");

  // Apply output back to segmentation image
  SegmentationUpdateIterator it_update(liw, liw->GetBufferedRegion()
                                       , m_Parent->GetGlobalState()->GetDrawingColorLabel()
                                       , m_Parent->GetGlobalState()->GetDrawOverFilter());
  itk::ImageRegionConstIterator<C3DImageType>
      it_src(c3dOutputImage, c3dOutputImage->GetBufferedRegion());

  for (; !it_update.IsAtEnd(); ++it_update, ++it_src)
      it_update.PaintLabel(it_src.Get());

  // Finalize update and create an undo point
  it_update.Finalize("Smooth Labels");

  // Fire events to inform GUI that segmentation has changed
  this->m_Parent->GetDriver()->InvokeEvent(SegmentationChangeEvent());
  liw->Modified();
}

void
SmoothLabelsModel
::Smooth(std::unordered_set<LabelType> &labelsToSmooth,
         std::vector<double> sigmaInput,
         SigmaUnit unit,
         bool SmoothAllFrames)
{
  // Get the segmentaton wrapper
  LabelImageWrapper *liw = m_Parent->GetDriver()->GetSelectedSegmentationLayer();


  unsigned int nT = liw->GetNumberOfTimePoints();

  // For 3D Image, It will always be 0;
  // For 4D Image, Smooth All will start from 0, otherwise current time point
  unsigned int crntFrame = SmoothAllFrames ? 0 : liw->GetTimePointIndex();

  // For 3D Image, It will always be 1
  // For 4D Image, Smooth All will end with last frame, otherwise before the next time point
  const unsigned int frameEnd = SmoothAllFrames ? nT : crntFrame + 1;

  // Stats calculator to determine if frame has segmentation to smooth
  typedef SegmentationStatistics::EntryMap EntryMap;

  SegmentationStatistics statsCalculator;
  const EntryMap &stats = statsCalculator.GetStats();

  // iteration through frames
  for (;crntFrame < frameEnd; ++crntFrame)
    {
      // Set current frame to target frame
      liw->SetTimePointIndex(crntFrame);

      // Compute Statistics
      statsCalculator.Compute(m_Parent->GetDriver());

      unsigned int cnt = 0;

      for (auto cit = stats.cbegin(); cit != stats.cend(); ++cit)
        {
          if (cit->second.count > 0)
            ++cnt;
        }

      // If there's only background label, skip the current frame
      if (cnt < 2)
        continue;

      // Apply the c3d multilabel smoothing api
      this->ApplyC3DSmoothing(liw, sigmaInput, unit, labelsToSmooth);
    }

  // Change label image to current frame
  liw->SetTimePointIndex(m_Parent->GetDriver()->GetCursorTimePoint());
  liw->Modified();
}
