/**
 * Define trainer class to deal with tree training processing in a depth first way,
 * forest training is just a lot of parallel tree training implemented by openMP.
 * Randomness is achieved by two ways:
 * randomness in sub-sample input dataset (bagging);
 * randomness in weak learner parameters chosen in each tree node, achieved by
 *   randomly choosing the best combination of weak classifier parameters (outside loop)
 *   and thresholds (inside loop) to get the largest information gain.
 */

#ifndef TRAINER_H
#define TRAINER_H

#include <limits>
#include "forest.h"
#include "trainingcontext.h"

template<class C, class S, class dataT, class labelT>
class Trainer
{
public:
  typedef DecisionTree<S,C,dataT> DecisionTreeT;
  typedef DecisionForest<S,C,dataT> DecisionForestT;

  Trainer(const MLData<dataT, labelT>& trainingData,
          TrainingParameters trainingParameters,
          TrainingContext<S, C>& trainingContext,
          Random& random)
    : trainingData_(trainingData),
      trainingParameters_(trainingParameters),
      trainingContext_(trainingContext),
      random_(random)
  {
    if (trainingParameters_.subSamplePercent == 0.0)
      {
        subSampleNum_ = trainingData_.Size();
      }
    else
      {
        subSampleNum_ = trainingData_.Size()
            * trainingParameters_.subSamplePercent / 100;
      }
  }

  size_t CandidateThresholds(std::vector<double>& thresholds,
                             std::vector<double>& featureResponses,
                             index_t begin, index_t end)
  {
    size_t thresholdNum;
    if ((end - begin) > trainingParameters_.candidateClassifierThresholdNum)
      {
        thresholdNum = trainingParameters_.candidateClassifierThresholdNum;
        for (size_t i = 0; i < (thresholdNum + 1); ++i)
          {
            thresholds[i] = featureResponses[random_.RandI(begin, end)];
          }
      }
    else
      {
        thresholdNum = end - begin - 1;
        std::copy(featureResponses.begin() + begin,
                  featureResponses.begin() + end,
                  thresholds.begin());
      }

    std::sort(thresholds.begin(), thresholds.begin() + thresholdNum + 1);

    if (thresholds[0] == thresholds[thresholdNum])
      {
        return 0;
      }
    else
      {
        for (size_t i = 0; i < thresholdNum; ++i)
          {
            thresholds[i] = thresholds[i] +
                random_.RandD() * (thresholds[i+1] - thresholds[i]);
          }
        return thresholdNum;
      }
  }

  void DepthFirst(DecisionTreeT& tree, Node* pnode,
                  bool side, size_t cDepth,
                  index_t begin, index_t end,
                  S& pStatistics, S& lStatistics, S& rStatistics,
                  std::vector<S>& ctStatistics,
                  std::vector<double>& cthresholds,
                  std::vector<size_t>& indices,
                  std::vector<double>& featureResponses,
                  std::vector<bool>& responses)
  {
    if (trainingParameters_.treeDepth == 1)
      {
        throw std::runtime_error("training parameters: treeDepth couldn't be 1\n");
      }
    if (tree.depth_ < cDepth)
      {
        tree.depth_ = cDepth;
      }
    Node* cNode;
    pStatistics.Clear();
    for(index_t i = begin; i < end; ++i)
      {
        pStatistics.Aggregate(trainingData_, indices[i]);
      }

    if (trainingParameters_.treeDepth > 0)
      {
        if (cDepth >= trainingParameters_.treeDepth)
          {
            cNode = tree.AddLeafNode(side, pnode, pStatistics,
                                     -std::numeric_limits<double>::infinity());
            return;
          }
      }

    size_t thresholdNum;
    double bestIG = 0.0;
    double cIG = 0.0;
    C bestClassifier = trainingContext_.RandomClassifier(random_);
    int maxTrial = 3;
    int trial = 0;
    while (trial < maxTrial)
      {
        for(size_t i = 0; i < trainingParameters_.candidateNodeClassifierNum; ++i)
          {
            C cClassifier = trainingContext_.RandomClassifier(random_);

            for (size_t j = 0; j < ctStatistics.size(); ++j)
              {
                ctStatistics[j].Clear();
              }

            for (index_t j = begin; j < end; ++j)
              {
                featureResponses[j] = cClassifier.FeatureResponse(trainingData_, indices[j]);
              }

            thresholdNum = 0;
            int counts = 0;
            while ((thresholdNum == 0) && (end - begin > 1) && (counts < 10))     /////////////
              {
                thresholdNum = CandidateThresholds(cthresholds, featureResponses, begin, end);
                counts++;
              }

            size_t which;
            for (index_t j = begin; j < end; ++j)
              {
                which = 0;
                while ((which < thresholdNum) &&
                       (featureResponses[j] >= cthresholds[which]))
                  {
                    ++which;
                  }
                ctStatistics[which].Aggregate(trainingData_, indices[j]);
              }

            for (size_t j = 0; j < thresholdNum; ++j)
              {
                lStatistics.Clear();
                rStatistics.Clear();

                for (size_t k = 0; k < (thresholdNum + 1); ++k)
                  {
                    if (k <= j)
                      {
                        lStatistics.Aggregate(ctStatistics[k]);
                      }
                    else
                      {
                        rStatistics.Aggregate(ctStatistics[k]);
                      }
                  }

                cIG = trainingContext_.ComputeIG(pStatistics, lStatistics, rStatistics,
                                                 trainingParameters_.weights);

                if (cIG >= bestIG)
                  {
                    bestIG = cIG;
                    bestClassifier = cClassifier;
                    bestClassifier.threshold_ = cthresholds[j];
                  }
              }
          }

        if (cDepth == 1)
          {
            cNode = tree.AddRoot(bestClassifier, pStatistics, bestIG);
            break;
          }
        else
          {
            if (bestIG <= trainingParameters_.splitIG)
              {
                if ((pStatistics.Entropy() <= trainingParameters_.leafEntropy) ||
                    (trainingParameters_.leafEntropy == -std::numeric_limits<double>::infinity()))
                  {
                    cNode = tree.AddLeafNode(side, pnode, pStatistics, bestIG);
                    return;
                  }
                else
                  {
                    ++trial;
                    if (trial == maxTrial)
                      {
                        ++tree.suspectLeaves_;
                        cNode = tree.AddLeafNode(side, pnode, pStatistics, bestIG);
                        return;
                      }
                    continue;
                  }
              }
            else
              {
                cNode = tree.AddSplitNode(side, pnode, bestClassifier, pStatistics, bestIG);
                break;
              }
          }
      }

    for (index_t i = begin; i < end; ++i)
      {
        responses[i] = bestClassifier.Response(trainingData_, indices[i]);
      }

    index_t division = Partition(responses, indices, begin, end);

    DepthFirst(tree, cNode, true, cDepth + 1, begin, division,
               pStatistics, lStatistics, rStatistics, ctStatistics,
               cthresholds, indices, featureResponses, responses);
    DepthFirst(tree, cNode, false, cDepth + 1, division, end,
               pStatistics, lStatistics, rStatistics, ctStatistics,
               cthresholds, indices, featureResponses, responses);
  }

  void Training(DecisionTreeT& tree)
  {
    size_t nodeNumBefore = tree.nodes_.size();
    if (nodeNumBefore != 0)
      {
        for (index_t i = 0; i < nodeNumBefore; ++i)
          {
            delete tree.nodes_[i];
          }
        tree.nodes_.resize(0);
      }

    S pStatistics, lStatistics, rStatistics;
    std::vector<S> ctStatistics;
    std::vector<double> cthresholds;
    std::vector<size_t> indices;
    std::vector<double> featureResponses;
    std::vector<bool> responses;

    indices.resize(subSampleNum_);
    if (subSampleNum_ == trainingData_.Size())
      {
        for(index_t i = 0; i < indices.size(); ++i)
          {
            indices[i] = i;
          }
      }
    else
      {
        for(index_t i = 0; i < indices.size(); ++i)
          {
            indices[i] = random_.RandD(0, trainingData_.Size());
          }
      }

    featureResponses.resize(subSampleNum_);
    responses.resize(subSampleNum_);

    pStatistics = trainingContext_.Statistics();
    lStatistics = trainingContext_.Statistics();
    rStatistics = trainingContext_.Statistics();

    ctStatistics.resize(trainingParameters_.candidateClassifierThresholdNum + 1);
    for(size_t i = 0; i < (trainingParameters_.candidateClassifierThresholdNum + 1); ++i)
      {
        ctStatistics[i] = trainingContext_.Statistics();
      }

    cthresholds.resize(trainingParameters_.candidateClassifierThresholdNum + 1); // only candidateClassifierThresholdNum is valid

    DepthFirst(tree, 0, true, 1, 0, subSampleNum_,
               pStatistics, lStatistics, rStatistics, ctStatistics,
               cthresholds, indices, featureResponses, responses);
  }

  void Training(DecisionForestT& forest)
  {
    size_t treeNumBefore = forest.trees_.size();
    if (treeNumBefore != 0)
      {
        for (index_t i = 0; i < treeNumBefore; ++i)
          {
            delete forest.trees_[i];
          }
        forest.trees_.resize(0);
      }

    for (index_t i = 0; i < trainingParameters_.treeNum; ++i)
      {
        forest.AddTree();
      }

    #pragma omp parallel for
    for (index_t i = 0; i < trainingParameters_.treeNum; ++i)
      {
        Training(*(forest.trees_[i]));
      }
  }

  const MLData<dataT, labelT>& trainingData_;
  TrainingParameters trainingParameters_;
  TrainingContext<S, C>& trainingContext_;
  size_t subSampleNum_;
  Random& random_;
};

#endif // TRAINER_H
