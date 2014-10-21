/**
 * Define decision forest as a vector of decision tree's pointer,
 * testing processing runs in parallel by openMP.
 */

#ifndef FOREST_H
#define FOREST_H

#include "tree.h"

template<class S, class C, class dataT>
class DecisionForest
{
public:
  typedef DecisionTree<S, C, dataT> DecisionTreeT;

  DecisionForest(): verbose_(false) {}
  DecisionForest(bool verbose): verbose_(verbose) {}
  ~DecisionForest()
  {
    for (size_t i = 0; i < trees_.size(); ++i)
      {
        delete trees_[i];
      }
  }

  DecisionTreeT* AddTree()
  {
    DecisionTreeT* ctree = new DecisionTreeT(verbose_);
    ctree->idx_ = trees_.size();
    trees_.push_back(ctree);
    return ctree;
  }

  void Apply(MLData<dataT, S*>& testingData,
             Vector<Vector<S*> >& testingResult)
  {
    size_t treeNum = trees_.size();
    size_t dataNum = testingData.Size();

    testingResult.Resize(treeNum);
    #pragma omp parallel for
    for (index_t i = 0; i < treeNum; ++i)
      {
        testingResult[i].Resize(dataNum);
        trees_[i]->Apply(testingData, testingResult[i]);
      }
  }

  // Added by Paul. This version of the method allows the same testingResult
  // vector to be reused. For speed, it does not check that the size of the
  // testing result is sufficient
  // This version of the method takes a preallocated vector
  // index and a preallocated vector response_vec of the same size as
  // testingData. It does not do allocation to save on computation time
  void ApplyFast(MLData<dataT, S*>& testingData,
                 Vector<Vector<S*> >& testingResult,
                 std::vector<index_t> &index,
                 std::vector<bool> &response)
  {
    size_t treeNum = trees_.size();
    for (index_t i = 0; i < treeNum; ++i)
        trees_[i]->ApplyFast(testingData, testingResult[i], index, response);
  }

  void Print(int level)
  {
    for (index_t i = 0; i < trees_.size(); ++i)
      {
        std::cout << "tree " << i << " information:" << std::endl;
        trees_[i]->Print(level);
      }
  }

  void Read(std::istream& is)
  {
    size_t treeNum = 0;
    readBasicType(is, verbose_);
    readBasicType(is, treeNum);

    size_t treeNumBefore = trees_.size();
    if (treeNumBefore != 0)
      {
        for (index_t i = 0; i < treeNumBefore; ++i)
          {
            delete trees_[i];
          }
      }
    trees_.resize(treeNum);
    for (index_t i = 0; i < treeNum; ++i)
      {
        trees_[i] = new DecisionTreeT(verbose_);
        trees_[i]->Read(is);
      }
  }

  void Write(std::ostream& os)
  {
    size_t treeNum = trees_.size();
    writeBasicType(os, verbose_);
    writeBasicType(os, treeNum);
    for (index_t i = 0; i < treeNum; ++i)
      {
        trees_[i]->Write(os);
      }
  }

  int GetForestSize() { return trees_.size(); }

  std::vector<DecisionTreeT*> trees_;
  bool verbose_;
};

#endif // FOREST_H
