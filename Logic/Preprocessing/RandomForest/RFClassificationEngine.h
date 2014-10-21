#ifndef RFCLASSIFICATIONENGINE_H
#define RFCLASSIFICATIONENGINE_H

#include <itkObject.h>
#include <itkObjectFactory.h>
#include "SNAPCommon.h"
#include "PropertyModel.h"

class SNAPImageData;
class RandomForestClassifier;

template <class TData, class TLabel> class MLData;

/**
 * This class serves as the high-level interface between ITK-SNAP and the
 * random forest code.
 */
class RFClassificationEngine : public itk::Object
{
public:

  // Standard ITK class stuff
  irisITKObjectMacro(RFClassificationEngine, itk::Object)

  /** Set the data source for the classification */
  void SetDataSource(SNAPImageData *imageData);

  /** Reset the classifier */
  void ResetClassifier();

  /** Train the classifier */
  void TrainClassifier();

  /** Set the classifier */
  void SetClassifier(RandomForestClassifier *rf);

  /** Access the trained classifier */
  irisGetMacro(Classifier, RandomForestClassifier *)

  /** Size of the random forest (main parameter) */
  irisGetSetMacro(ForestSize, int)

  /** Get the number of components passed to the classifier */
  int GetNumberOfComponents() const;


protected:

  RFClassificationEngine();
  virtual ~RFClassificationEngine();

  // The trained classifier
  SmartPtr<RandomForestClassifier> m_Classifier;

  // The data source
  SNAPImageData *m_DataSource;

  // The foreground label
  LabelType m_ForegroundLabel;

  // Number of trees
  int m_ForestSize;

  // Cached samples used to train the classifier
  typedef MLData<GreyType, LabelType> SampleType;
  SampleType *m_Sample;

};

#endif // RFCLASSIFICATIONENGINE_H
