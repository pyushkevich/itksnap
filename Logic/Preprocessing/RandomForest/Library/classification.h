/**
 * Solve classification problem by random forest.
 */

#ifndef CLASSIFICATION_H
#define CLASSIFICATION_H

#include <set>
#include <map>
#include "trainer.h"

// Bug - Paul (changed labelT to be a parameter)
template<class dataT, class labelT, class ClassifierT>
class Classification
{
public:
  // typedef int labelT;
  typedef Histogram<dataT, labelT> HistStatisticsT;
  typedef ClassificationContext<ClassifierT, dataT, labelT> ClassificationContextT;
  typedef DecisionForest<HistStatisticsT, ClassifierT, dataT> DecisionForestT;
  typedef Trainer<ClassifierT, HistStatisticsT, dataT, labelT> TrainerT;

  // Bug - Paul (changed int to labelT)
  typedef MLData<dataT, labelT> TrainingDataT;
  typedef MLData<dataT, HistStatisticsT*> TestingDataT;
  typedef Vector<Vector<HistStatisticsT*> > TestingResultT;
  typedef Matrix<double> SoftPredictionT;
  typedef Vector<int> HardPredictionT;

  bool ValidData(TrainingDataT& trainingData,
                 std::map<index_t, labelT>& mapping)
  {
    std::set<int> labelSet(trainingData.label.Begin(),
                           trainingData.label.End());
    std::set<int>::iterator setIter;
    std::map<int, index_t> backMapping;
    mapping.clear();
    index_t i = 0;
    bool valid = true;
    for (setIter = labelSet.begin(); setIter != labelSet.end(); ++setIter, ++i)
      {
        backMapping.insert(std::map<int, index_t>::value_type(*setIter, i));
        mapping.insert(std::map<index_t, int>::value_type(i, *setIter));
        if (*setIter != i)
          {
            valid = false;
          }
      }
    if (valid == false)
      {
        std::map<int, index_t>::iterator backMappingIter;
        for (index_t i = 0; i < trainingData.Size(); ++i)
          {
            backMappingIter = backMapping.find(trainingData.label[i]);
            if (backMappingIter == backMapping.end())
              {
                throw std::runtime_error("Classificaiton: ValidData error");
              }
            else
              {
                trainingData.label[i] = backMappingIter->second;
              }
          }
      }
    return valid;
  }

  // Bug: paul - int should be labelT
  void Learning(TrainingParameters& trainingParameters,
                TrainingDataT& trainingData,
                DecisionForestT& forest,
                bool& validLabel,
                std::map<index_t, labelT>& mapping)
  {
    validLabel = ValidData(trainingData, mapping);
    size_t classNum = mapping.size();
    if ( !trainingParameters.weights.empty() )
      {
        if (classNum != trainingParameters.weights.size())
          {
            throw std::runtime_error("Traing parameter weights size doesn't match class size in data\n");
          }
      }

    Random random;
    ClassificationContextT classificationTC(trainingData.Dimension(), classNum);
    TrainerT trainer(trainingData, trainingParameters, classificationTC, random);

    trainer.Training(forest);
  }

  void Predicting(DecisionForestT& forest,
                  TestingDataT& testingData,
                  bool& validLabel, std::map<index_t, int>& mapping,
                  SoftPredictionT& softPrediction,
                  HardPredictionT&  hardPrediction)
  {
    TestingResultT testingResult;
    forest.Apply(testingData, testingResult);

    size_t classNum = mapping.size();
    size_t treeNum = testingResult.Size();
    size_t dataNum = testingResult[0].Size();
    std::vector<double> max(dataNum, 0);

    #pragma omp parallel for
    for (index_t i = 0; i < dataNum; ++i)
      {
        for (index_t j = 0; j < classNum; ++j)
          {
            for (index_t k = 0; k < treeNum; ++k)
              {
                if (k == 0)
                  {
                    softPrediction[i][j] = testingResult[k][i]->prob_[j];
                  }
                else
                  {
                    softPrediction[i][j] = softPrediction[i][j] + testingResult[k][i]->prob_[j];
                  }
              }
            softPrediction[i][j] = softPrediction[i][j] / treeNum;
            if (softPrediction[i][j] > max[i])
              {
                max[i] = softPrediction[i][j];
                hardPrediction[i] = j;
              }
          }
        if (validLabel == false)
          {
            // Bug: typename missing
            typename std::map<index_t, labelT>::iterator mapIter = mapping.find(hardPrediction[i]);
            if (mapIter == mapping.end())
              {
                throw std::runtime_error("Classificaiton: Predicting mapping error");
              }
            else
              {
                hardPrediction[i] = mapIter->second;
              }
          }
      }

  }

  void Run(TrainingParameters& trainingParameters,
           TrainingDataT& trainingData,
           TestingDataT& testingData,
           DecisionForestT& forest,
           std::map<index_t, int>& mapping,
           SoftPredictionT& softPrediction,
           HardPredictionT& hardPrediction)
  {
    bool validLabel = true;
    MPTimer timer;
    std::cerr << "training begin\n";
    timer.Start();
    Learning(trainingParameters, trainingData, forest, validLabel, mapping);
    std::cerr << "training finished, spending " << timer.StopAndSpendSecond() << " secs\n";
    std::cerr << "testing begin\n";
    timer.Start();
    Predicting(forest, testingData, validLabel, mapping, softPrediction, hardPrediction);
    std::cerr << "testing finished, spending " << timer.StopAndSpendSecond() << " secs\n";
  }
};

#endif // CLASSIFICATION_H
