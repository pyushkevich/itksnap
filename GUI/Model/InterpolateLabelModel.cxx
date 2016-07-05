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

#include "itkCastImageFilter.h"

#include "ConvertAPI.h"

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
  GenericImageData *id = this->m_Parent->GetDriver()->GetCurrentImageData();
  LabelImageWrapper *liw = id->GetSegmentation();

  // Cast to an image of float
  typedef LabelImageWrapper::DoubleImageSource DoubleImageSource;
  SmartPtr<DoubleImageSource> caster = liw->CreateCastToDoublePipeline();
  caster->Update();

  // We will call C3D for this
  typedef ConvertAPI<double,3> APIType;
  APIType api;
  api.AddImage("S", caster->GetOutput());

  // Create a command
  char command[4096];

  if(this->GetUseLevelSet())
    {
    sprintf(command,"c3d \
           -verbose -push S \
           -threshold %d %d 1 0 \
           -as R -trim 4vox -as T \
           -dilate 0 0x1x1 -dilate 1 0x3x3 \
           -push T -dilate 0 1x0x1 -dilate 1 3x0x3 \
           -push T -dilate 0 1x1x0 -dilate 1 3x3x0 \
           -add -add -thresh 1 inf -1 0 -push T -scale 2 -add -as SROI \
           -insert R 1 -reslice-identity -as SPEED -clear \
           -push SROI -thresh 1 1 1 0 -sdt -smooth %fvox \
           -push SROI -thresh -1 -1 1 0 -sdt -smooth %fvox \
           -vote -insert R 1 -reslice-identity -as INIT \
           -type char \
           -clear -push SPEED -shift 0.02 -push INIT \
           -replace 1 -1 0 1 -levelset-curvature %f -levelset 400 \
           -thresh -inf 0 1 0 \
           -as Z",
            this->GetInterpolateLabel(), this->GetInterpolateLabel(),
            this->GetSmoothing(), this->GetSmoothing(),
            this->GetLevelSetCurvature());
    }
  else
    {
    sprintf(command,"c3d \
            -push S -info -thresh %d %d 1 0 -info -as R -trim 4vox -info -as T \
            -dilate 0 0x1x1 -dilate 1 0x400x400 \
            -push T -dilate 0 1x0x1 -dilate 1 400x0x400 \
            -push T -dilate 0 1x1x0 -dilate 1 400x400x0 \
            -add -add -thresh 1 inf -1 0 -push T -scale 2 -add -as SROI \
            -insert R 1 -reslice-identity -as SPEED -clear \
            -push SROI -thresh 1 1 1 0 -sdt -smooth %fvox \
            -push SROI -thresh -1 -1 1 0 -sdt -smooth %fvox \
            -vote -insert R 1 -reslice-identity \
            -as Z",
            this->GetInterpolateLabel(), this->GetInterpolateLabel(),
            this->GetSmoothing(), this->GetSmoothing());
    }

  // Execute the command
  bool rc = api.Execute(command, std::cout);
  SmartPtr<APIType::ImageType> image = api.GetImage("Z");

  // Did we get a result?
  if(rc && image)
    {
    SegmentationUpdateIterator it_trg(liw->GetImage(), liw->GetImage()->GetBufferedRegion(),
                                     this->GetDrawingLabel(), this->GetDrawOverFilter());

    itk::ImageRegionConstIterator<APIType::ImageType>it_src(image, image->GetBufferedRegion());

    bool clear_scaffold = !this->GetRetainScaffold();
    LabelType label_to_clear = this->GetInterpolateLabel();
    for(; !it_trg.IsAtEnd(); ++it_trg, ++it_src)
      {
      if(it_src.Value())
        it_trg.PaintAsForeground();
      else if(clear_scaffold)
        it_trg.ReplaceLabel(label_to_clear, 0);
      }

    id->StoreUndoPoint("Interpolate label", it_trg.RelinquishDelta());
    id->InvokeEvent(SegmentationChangeEvent());
    }

}

InterpolateLabelModel::InterpolateLabelModel()
{
  m_InterpolateAllModel = NewSimpleConcreteProperty(false);

  m_DrawingLabelModel = ConcreteColorLabelPropertyModel::New();
  m_InterpolateLabelModel = ConcreteColorLabelPropertyModel::New();
  m_DrawOverFilterModel = ConcreteDrawOverFilterPropertyModel::New();

  m_SmoothingModel = NewRangedConcreteProperty(3.0, 0.0, 20.0, 0.01);
  m_LevelSetCurvatureModel = NewRangedConcreteProperty(0.2, 0.0, 1.0, 0.01);

  m_RetainScaffoldModel = NewSimpleConcreteProperty(false);
  m_UseLevelSetModel = NewSimpleConcreteProperty(false);
}
