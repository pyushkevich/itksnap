#include "InterpolateLabelModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ScalarImageWrapper.h"
#include "itkMorphologicalContourInterpolator.h"
#include "SegmentationUpdateIterator.h"
#include "itkBinaryThresholdImageFilter.h"

void InterpolateLabelModel::SetParentModel(GlobalUIModel *parent)
{
  this->m_Parent = parent;

  m_DrawingLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_InterpolateLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());
  m_DrawOverFilterModel->Initialize(parent->GetDriver()->GetColorLabelTable());
}

void InterpolateLabelModel::UpdateOnShow()
{
  // What we want to do here is syncronize labels with existing selections for active and draw over
  this->SetDrawingLabel(m_Parent->GetGlobalState()->GetDrawingColorLabel());
  this->SetInterpolateLabel(m_Parent->GetGlobalState()->GetDrawingColorLabel());
  this->SetDrawOverFilter(m_Parent->GetGlobalState()->GetDrawOverFilter());
}

template <class TLabel>
class BinarizeFunctor
{
public:
  void SetLabel(const TLabel &l) { m_Label = l; }
  
  TLabel operator()(const TLabel &in) { return (in == m_Label) ? m_Label : 0; }
  bool operator != (const BinarizeFunctor<TLabel> &other ) { return m_Label != other.m_Label; }
private:
  TLabel m_Label;
};

void InterpolateLabelModel::Interpolate()
{
  // Get the segmentation wrapper
  LabelImageWrapper *liw = m_Parent->GetDriver()->GetSelectedSegmentationLayer();

  // Create the morphological interpolation filter
  typedef itk::MorphologicalContourInterpolator<GenericImageData::LabelImageType> MCIType;
  SmartPtr<MCIType> mci = MCIType::New();

  // Are we interpolating all labels?
  bool interp_all = this->GetInterpolateAll();

  // Should we be interpolating a specific label or all labels?
  if(interp_all)
    {
    mci->SetInput(liw->GetImage());
    }
  else
    {
    // We need to extract a single component from the segmentation image to interpolate
    typedef GenericImageData::LabelImageType LabelImageType;
    typedef BinarizeFunctor<LabelType> FunctorType;
    typedef itk::UnaryFunctorImageFilter<LabelImageType, LabelImageType, FunctorType> BinarizeFilterType;
    BinarizeFilterType::Pointer flt = BinarizeFilterType::New();
    
    FunctorType fn;
    fn.SetLabel(this->GetInterpolateLabel());
    flt->SetInput(liw->GetImage());
    flt->SetFunctor(fn);
    flt->Update();

    mci->SetInput(flt->GetOutput());
    mci->SetLabel(this->GetInterpolateLabel());
    }

  // Should we interpolate only one axis?
  if (this->GetMorphologyInterpolateOneAxis())
    {
    int axis = this->m_Parent->GetDriver()->GetImageDirectionForAnatomicalDirection(this->GetMorphologyInterpolationAxis());
    mci->SetAxis(axis);
    }

  // Should we use the distance transform?
  mci->SetUseDistanceTransform(this->GetMorphologyUseDistance());

  // Should heuristic or optimal alignment be used?
  mci->SetHeuristicAlignment(!this->GetMorphologyUseOptimalAlignment());

  // Update the filter
  mci->Update();
  
  // Apply the labels back to the segmentation
  SegmentationUpdateIterator it_trg(liw->GetImage(), liw->GetImage()->GetBufferedRegion(),
                                    this->GetDrawingLabel(), this->GetDrawOverFilter());

  itk::ImageRegionConstIterator<GenericImageData::LabelImageType>
      it_src(mci->GetOutput(), mci->GetOutput()->GetBufferedRegion());

  // The way we paint back into the segmentation depends on whether all labels
  // or a specific label are being interpolated
  if(interp_all)
    {
    // Just replace the segmentation by the interpolation, respecting draw-over
    for(; !it_trg.IsAtEnd(); ++it_trg, ++it_src)
      it_trg.PaintLabel(it_src.Get());
    }
  else
    {
    LabelType l_interp = this->GetInterpolateLabel();
    LabelType l_replace = this->GetDrawingLabel();
    for(; !it_trg.IsAtEnd(); ++it_trg, ++it_src)
      if(it_src.Get() == l_interp)
        it_trg.PaintLabelWithExtraProtection(l_interp, l_replace);
    }

  // Finish the segmentation editing and create an undo point
  it_trg.Finalize();
  liw->StoreUndoPoint("Interpolate label", it_trg.RelinquishDelta());

  // Fire event to inform GUI that segmentation has changed
  this->m_Parent->GetDriver()->InvokeEvent(SegmentationChangeEvent());
}


InterpolateLabelModel::InterpolateLabelModel()
{
  m_InterpolateAllModel = NewSimpleConcreteProperty(false);

  m_DrawingLabelModel = ConcreteColorLabelPropertyModel::New();
  m_InterpolateLabelModel = ConcreteColorLabelPropertyModel::New();
  m_DrawOverFilterModel = ConcreteDrawOverFilterPropertyModel::New();
  m_RetainScaffoldModel = NewSimpleConcreteProperty(false);

  RegistryEnumMap<InterpolationType> emap_interp;
  emap_interp.AddPair(MORPHOLOGY,"Morphological");
  emap_interp.AddPair(LEVEL_SET,"Level set");
  emap_interp.AddPair(DISTANCE_MAP,"Distance map");
  m_InterpolationMethodModel = NewSimpleEnumProperty("InterpolationType", MORPHOLOGY, emap_interp);

  m_DefaultSmoothingModel = NewRangedConcreteProperty(3.0, 0.0, 20.0, 0.01);

  m_LevelSetSmoothingModel = NewRangedConcreteProperty(3.0, 0.0, 20.0, 0.01);
  m_LevelSetCurvatureModel = NewRangedConcreteProperty(0.2, 0.0, 1.0, 0.01);

  m_MorphologyUseDistanceModel = NewSimpleConcreteProperty(false);
  m_MorphologyUseOptimalAlignmentModel = NewSimpleConcreteProperty(false);
  m_MorphologyInterpolateOneAxisModel = NewSimpleConcreteProperty(false);

  RegistryEnumMap<AnatomicalDirection> emap_interp_axis;
  emap_interp_axis.AddPair(ANATOMY_AXIAL,"Axial");
  emap_interp_axis.AddPair(ANATOMY_SAGITTAL,"Sagittal");
  emap_interp_axis.AddPair(ANATOMY_CORONAL,"Coronal");
  m_MorphologyInterpolationAxisModel = NewSimpleEnumProperty("InterpolationAxis", ANATOMY_AXIAL, emap_interp_axis);
}
