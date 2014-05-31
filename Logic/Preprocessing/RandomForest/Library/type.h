#ifndef TYPE_H
#define TYPE_H

#include "classification.h"

typedef AxisAlignedClassifier<dataT, labelT> AxisClassifierT;
typedef LinearClassifier<dataT, labelT> LinearClassifierT;
typedef Histogram<dataT, labelT> HistStatisticsT;
typedef DecisionForest<HistStatisticsT, AxisClassifierT, dataT> ClassificationForestAxisT;
typedef DecisionForest<HistStatisticsT, LinearClassifierT, dataT> ClassificationForestLinearT;
typedef Classification<dataT, AxisClassifierT> ClassificationAxisT;
typedef Classification<dataT, LinearClassifierT> ClassificationLinearT;
typedef Matrix<double> SoftPredictionT;
typedef Vector<labelT> HardPredictionT;

#endif // TYPE_H
