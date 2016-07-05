#ifndef RFCLASSIFICATIONENGINE_H
#define RFCLASSIFICATIONENGINE_H

#include <itkObject.h>
#include <itkObjectFactory.h>
#include "SNAPCommon.h"
#include <itkSize.h>

template <class TPixel, class TLabel, int VDim> class RandomForestClassifier;
template <class TData, class TLabel> class MLData;
class SNAPImageData;

/**
 * This class serves as the high-level interface between ITK-SNAP and the
 * random forest code.
 */
template <class TPixel, class TLabel, int VDim>
class RFClassificationEngine : public itk::Object
{
public:

  // Standard ITK class stuff
  irisITKObjectMacro(RFClassificationEngine, itk::Object)

  // Patch radius type
  typedef itk::Size<VDim> RadiusType;

  // Classifier type
  typedef RandomForestClassifier<TPixel, TLabel, VDim> ClassifierType;

  /** Set the data source for the classification */
  void SetDataSource(SNAPImageData *imageData);

  /** Reset the classifier */
  void ResetClassifier();

  /** Train the classifier */
  void TrainClassifier();

  /** Set the classifier */
  void SetClassifier(ClassifierType *rf);

  /** Access the trained classifier */
  itkGetMacro(Classifier, ClassifierType *)

  /** Size of the random forest (main parameter) */
  itkGetMacro(ForestSize, int)
  itkSetMacro(ForestSize, int)

  /** Size of the random forest (main parameter) */
  itkGetMacro(TreeDepth, int)
  itkSetMacro(TreeDepth, int)

  /** Patch radius for sampling features */
  itkGetMacro(PatchRadius, const RadiusType &)
  itkSetMacro(PatchRadius, RadiusType)

  /** Whether coordinates of the voxels are used as features */
  itkGetMacro(UseCoordinateFeatures, bool)
  itkSetMacro(UseCoordinateFeatures, bool)

  /** Get the number of components passed to the classifier */
  int GetNumberOfComponents() const;


protected:

  RFClassificationEngine();
  virtual ~RFClassificationEngine();

  // The trained classifier
  SmartPtr<ClassifierType> m_Classifier;

  // The data source
  SNAPImageData *m_DataSource;

  // The foreground label
  LabelType m_ForegroundLabel;

  // Number of trees
  int m_ForestSize;

  // Number of trees
  int m_TreeDepth;

  // Patch radius
  RadiusType m_PatchRadius;

  // Are coordinates included as features
  bool m_UseCoordinateFeatures;

  // Cached samples used to train the classifier
  typedef MLData<GreyType, LabelType> SampleType;
  SampleType *m_Sample;

};

#endif // RFCLASSIFICATIONENGINE_H
