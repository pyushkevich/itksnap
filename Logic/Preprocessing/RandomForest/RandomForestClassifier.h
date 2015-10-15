#ifndef RANDOMFORESTCLASSIFIER_H
#define RANDOMFORESTCLASSIFIER_H

#include <itkDataObject.h>
#include <itkObjectFactory.h>
#include <itkSize.h>
#include <SNAPCommon.h>
#include <map>

template <class dataT, class labelT> class Histogram;
template <class dataT, class labelT> class AxisAlignedClassifier;
template <class HistT, class ClassT, class dataT> class DecisionForest;

/**
 * This class encapsulates a Random Forest classifier
 */
class RandomForestClassifier : public itk::DataObject
{
public:

  // Standard ITK stuff
  irisITKObjectMacro(RandomForestClassifier, itk::DataObject)

  // typedefs
  typedef Histogram<GreyType, LabelType> RFHistogramType;
  typedef AxisAlignedClassifier<GreyType, LabelType> RFAxisClassifierType;
  typedef DecisionForest<RFHistogramType, RFAxisClassifierType, GreyType> RandomForestType;
  typedef std::map<size_t, LabelType> MappingType;
  typedef itk::Size<3> SizeType;

  // A list of weights for each class - used to construct speed image
  typedef std::vector<double> WeightArray;

  // Reset the classifier
  void Reset();

  // Get the mapping from the class indices to labels
  irisGetMacro(ClassToLabelMapping, const MappingType &)

  // Get the random forest
  irisGetMacro(Forest, RandomForestType *)

  // Get the patch radius
  irisGetMacro(PatchRadius, const SizeType &)

  /** Whether coordinates of the voxels are used as features */
  itkGetMacro(UseCoordinateFeatures, bool)
  itkSetMacro(UseCoordinateFeatures, bool)

  // Set the bias parameter (adjusts the mapping of FG probability to speed)
  itkGetMacro(BiasParameter, double)
  itkSetMacro(BiasParameter, double)

  // Get a reference to the weight array
  irisGetMacro(ClassWeights, const WeightArray &)

  // Set the weight for a class
  void SetClassWeight(size_t class_id, double weight);

  // Test if the classifier is valid (has 2+ classes)
  bool IsValidClassifier() const;

protected:

  RandomForestClassifier();
  ~RandomForestClassifier();

  // The actual decision forest
  RandomForestType *m_Forest;

  // Whether the labels are valid (?)
  bool m_ValidLabel;

  // Mapping of index to label (?)
  MappingType m_ClassToLabelMapping;

  // Weight of each class
  WeightArray m_ClassWeights;

  // The patch radius
  SizeType m_PatchRadius;

  // Whether coordinate features are used
  bool m_UseCoordinateFeatures;

  // Bias parameter
  double m_BiasParameter;

  // Let the engine handle our data
  friend class RFClassificationEngine;
};

#endif // RANDOMFORESTCLASSIFIER_H
