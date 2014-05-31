#ifndef RANDOMFORESTCLASSIFIER_H
#define RANDOMFORESTCLASSIFIER_H

#include <itkDataObject.h>
#include <itkObjectFactory.h>
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

  // Reset the classifier
  void Reset();


  RandomForestClassifier();
  ~RandomForestClassifier();

  // The actual decision forest
  RandomForestType *m_Forest;

  // Whether the labels are valid (?)
  bool m_ValidLabel;

  // Mapping of index to label (?)
  MappingType m_Mapping;

  // Let the engine handle our data
  friend class RFClassificationEngine;

  // TODO: make all that protected!
protected:

};

#endif // RANDOMFORESTCLASSIFIER_H
