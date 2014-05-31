/**
 * Define tree structure as a vector of nodes' pointer,
 * member nodes_ is stored as depth first way, as well as Travel member function,
 * add node into tree should be done through AddRoot, AddSplitNode or AddLeafNode,
 * streaming and serialization interface are included into DecisionTree class.
 *
 * Partition define as a public function that divides parent node's data into
 * its left and right child node according to pre-calculated boolean response.
 */

#ifndef TREE_H
#define TREE_H

#include <iomanip>
#include <queue>
#include <stdexcept>
#include <cmath>
#include "node.h"
#include "data.h"

// paul: bug, this function needs inlining
inline std::size_t Partition(std::vector<bool>& response,
                      std::vector<index_t>& index,
                      index_t begin,
                      index_t end)
{
  index_t i = begin;
  index_t j = end-1;
  index_t tmpIdx;
  bool tmpResps;
  while (i != j)
    {
      while ((response[i] == false) && (i != j))
        {
          ++i;
        }
      while ((response[j] == true) && (i != j))
        {
          --j;
        }
      if (i != j)
        {
          tmpIdx = index[i];
          tmpResps = response[i];
          index[i] = index[j];
          response[i] = response[j];
          index[j] = tmpIdx;
          response[j] = tmpResps;
        }
    }

  return (response[i] == false? i+1 : i);
}

template<class S, class C, class dataT>
class DecisionTree
{
public:
  typedef DTLeaf<S> LeafT;
  typedef DTSplit<C> SplitT;
  typedef DTSplitFull<S, C> SplitFullT;

  DecisionTree(bool verbose): verbose_(verbose), depth_(0), suspectLeaves_(0) {}
  ~DecisionTree()
  {
    for (size_t i = 0; i < nodes_.size(); ++i)
      {
        delete nodes_[i];
      }
  }

  void Apply(MLData<dataT, S*>& testingData,
             Vector<S*>& testingResult)
  {
    std::vector<index_t> index(testingData.Size());
    for (index_t i = 0; i != testingData.Size(); ++i)
      {
        index[i] = i;
      }

    std::vector<bool> response(testingData.Size());

    Travel(nodes_[0], 0, testingData.Size(), testingData, testingResult,
           index, response);
  }

  // Added by Paul. This version of the method takes a preallocated vector
  // index and a preallocated vector response_vec of the same size as
  // testingData. It does not do allocation to save on computation time
  void ApplyFast(MLData<dataT, S*>& testingData,
                 Vector<S*>& testingResult,
                 std::vector<index_t> &index,
                 std::vector<bool> &response)
  {
    for (index_t i = 0; i != testingData.Size(); ++i)
      {
        index[i] = i;
      }

    Travel(nodes_[0], 0, testingData.Size(), testingData, testingResult,
           index, response);
  }

  // depth first travel
  void Travel(Node* node, index_t begin, index_t end,
              MLData<dataT, S*>& testingData, Vector<S*>& testingResult,
              std::vector<index_t>& index, std::vector<bool>& response)
  {
    if (begin == end)
      {
        return;
      }

    if (node->IsLeaf())
      {
        for (index_t i = begin; i != end; ++i)
          {
            testingResult[index[i]] = &(((LeafT*)node)->statistics_);
          }
        return;
      }

//    #pragma omp parallel for
    for (index_t i = begin; i < end; ++i)
      {
        response[i] = ((SplitT*)node)->classifier_.Response(testingData, index[i]);
      }

    std::size_t division = Partition(response, index, begin, end);

    Travel(((SplitT*)node)->leftChild_, begin, division,
           testingData, testingResult, index, response);
    Travel(((SplitT*)node)->rightChild_, division, end,
           testingData, testingResult, index, response);
  }

  Node* AddLeafNode(bool side, Node* parent, S& statistics, double gain)
  {
    Node* cnode = (Node*) new LeafT(statistics);
    cnode->type_ = 'l';
    cnode->parent_ = parent;
    cnode->idx_ = nodes_.size();
    cnode->parentIdx_ = parent->idx_;
    ((LeafT*)cnode)->informationGain_ = gain;
    if (side == true)
      {
        ((SplitNode*)parent)->leftChild_ = cnode;
        ((SplitNode*)parent)->leftChildIdx_ = cnode->idx_;
      }
    else
      {
        ((SplitNode*)parent)->rightChild_ = cnode;
        ((SplitNode*)parent)->rightChildIdx_ = cnode->idx_;
      }
    nodes_.push_back(cnode);
    return cnode;
  }

  Node* AddSplitNode(bool side, Node* parent, C& classifier, S& statistics, double gain)
  {
    Node* cnode = 0;
    if (verbose_)
      {
        cnode = (Node*) new SplitFullT(statistics, classifier);
      }
    else
      {
        cnode = (Node*) new SplitT(classifier);
      }
    cnode->type_ = 's';
    cnode->parent_ = parent;
    cnode->idx_ = nodes_.size();
    cnode->parentIdx_ = parent->idx_;
    ((SplitT*)cnode)->informationGain_ = gain;
    if (side == true)
      {
        ((SplitNode*)parent)->leftChild_ = cnode;
        ((SplitNode*)parent)->leftChildIdx_ = cnode->idx_;
      }
    else
      {
        ((SplitNode*)parent)->rightChild_ = cnode;
        ((SplitNode*)parent)->rightChildIdx_ = cnode->idx_;
      }
    nodes_.push_back(cnode);
    return cnode;
  }

  Node* AddRoot(C& classifier, S& statistics, double gain)
  {
    Node* cnode = 0;
    if (verbose_)
      {
        cnode = (Node*) new SplitFullT(statistics, classifier);
      }
    else
      {
        cnode = (Node*) new SplitT(classifier);
      }
    cnode->type_ = 'r';
    cnode->parent_ = 0;
    cnode->idx_ = nodes_.size();
    cnode->parentIdx_ = -1;
    ((SplitT*)cnode)->informationGain_ = gain;
    nodes_.push_back(cnode);
    return cnode;
  }

//  void BreadthFirstTraversal()
//  {
//    nodesBreadthFirst_.resize(nodes_.size());
//    std::queue<Node*> unvisited;
//    Node* cNode;
//    unvisited.push(nodes_[0]);
//    int idx = 0;
//    while (!unvisited.empty())
//      {
//        cNode = unvisited.front();
//        if (!cNode->IsLeaf())
//          {
//            if (((SplitNode*)cNode)->leftChild != 0)
//              {
//                unvisited.push(((SplitNode*)cNode)->leftChild);
//              }
//            if (((SplitNode*)cNode)->rightChild != 0)
//              {
//                unvisited.push(((SplitNode*)cNode)->rightChild);
//              }
//          }
//        nodesBreadthFirst_[idx++] = cNode;
//        unvisited.pop();
//      }
//  }

  void PreOrderDepthFirst(Node* cNode, int cIndex)
  {
    breadthFirstFullTree_[cIndex-1] = cNode;
    if (cNode->IsLeaf())
      {
        return;
      }
    PreOrderDepthFirst(((SplitNode*)cNode)->leftChild_, cIndex * 2);
    PreOrderDepthFirst(((SplitNode*)cNode)->rightChild_, cIndex * 2 + 1);
  }

  void BreathFirstFullTree()
  {
    breadthFirstFullTree_.resize(pow(2, depth_)-1);
    for (int i = 0; i < breadthFirstFullTree_.size(); ++i)
      {
        breadthFirstFullTree_[i] = 0;
      }
    PreOrderDepthFirst(nodes_[0], 1);
  }

  void DepthFirstPrint(Node* cNode, size_t cDepth, bool side, std::string str)
  {
    if (cNode->type_ == 'r')
      {
        std::cout << str << "R " << cNode->idx_
                  << std::setprecision(4)
                  << ":  ("
//                  << "threshold = "
//                  << ((SplitT*)cNode)->classifier_.threshold_
//                  << ", "
                  << "IG = "
                  << ((SplitT*)cNode)->informationGain_
                  << ")"<< std::endl;
      }
    else
      {
        if (cNode->type_ == 'l')
          {
            if (((LeafT*)cNode)->informationGain_ == -std::numeric_limits<double>::infinity())
              {
                if (side == true)
                  {
                    std::cout << str << "|-L " << cNode->idx_
                              << ":  Reach max tree depth (H = "
                              << ((LeafT*)cNode)->statistics_.Entropy()
                              << ")"<< std::endl;
                  }
                else
                  {
                    std::cout << str << "+-L " << cNode->idx_
                              << ":  Reach max tree depth (H = "
                              << ((LeafT*)cNode)->statistics_.Entropy()
                              << ")"<< std::endl;
                  }
              }
            else
              {
                if (side == true)
                  {
                    std::cout << str << "|-L " << cNode->idx_
                              << ":  Small IG = "
                              << std::setiosflags(std::ios::fixed)
                              << std::setprecision(2)
                              << ((SplitT*)cNode)->informationGain_
                              << " (H = "
                              << ((LeafT*)cNode)->statistics_.Entropy()
                              << ")"<< std::endl;
                  }
                else
                  {
                    std::cout << str << "+-L " << cNode->idx_
                              << ":  Small IG = "
                              << std::setiosflags(std::ios::fixed)
                              << std::setprecision(2)
                              << ((SplitT*)cNode)->informationGain_
                              << " (H = "
                              << ((LeafT*)cNode)->statistics_.Entropy()
                              << ")"<< std::endl;
                  }
              }
            return;
          }
        else
          {
            if (side == true)
              {
                std::cout << str << "|-S " << cNode->idx_
                          << std::setprecision(4)
                          << ":  ("
//                          << "threshold = "
//                          << ((SplitT*)cNode)->classifier_.threshold_
//                          << ", "
                          << "IG = "
                          << ((SplitT*)cNode)->informationGain_
                          << ")"<< std::endl;
              }
            else
              {
                std::cout << str << "+-S " << cNode->idx_
                          << std::setprecision(4)
                          << ":  ("
//                          << "threshold = "
//                          << ((SplitT*)cNode)->classifier_.threshold_
//                          << ", "
                          << "IG = "
                          << ((SplitT*)cNode)->informationGain_
                          << ")"<< std::endl;
              }
          }
      }
    if (cDepth > 1)
      {
        if (side == true)
          {
            str += "| ";
          }
        else
          {
            str += "  ";
          }
      }
    DepthFirstPrint(((SplitNode*)cNode)->leftChild_, cDepth+1, true, str);
    DepthFirstPrint(((SplitNode*)cNode)->rightChild_, cDepth+1, false, str);
  }

  void Print(int level)
  {
    if ((level % 1000) > 0)
      {
        for (size_t i = 0; i < nodes_.size(); ++i)
          {
            nodes_[i]->Print(level);
          }
      }
    if ((level/1000 - (level/10000)*10)  == 1)
      {
        DepthFirstPrint(nodes_[0], 1, true, std::string(""));
      }
    if ((level/1000 - (level/10000)*10)  == 2)
      {
        if ((breadthFirstFullTree_.size() == 0) && (nodes_.size() != 0))
          {
            BreathFirstFullTree();
          }
        if (breadthFirstFullTree_.size() != 0)
          {
            int spaceWidth = 1;
            int numWidth = 0;
            int s = nodes_.size();
            while (s != 0)
              {
                numWidth++;
                s /= 10;
              }

            std::ios_base::fmtflags original_flags = std::cout.flags();
            int index = 0;
            int interval = pow(2, depth_) - 1;
            int start = (interval - 1) / 2;

            int nodeNum = 0;
            for (int i = 0; i < depth_; ++i)
              {
                nodeNum = pow(2, i);
                for (int j = 0; j < start; ++j)
                  {
                    for (int k = 0; k < (spaceWidth + numWidth); ++k)
                      {
                        std::cout << " ";
                      }
                  }
                for (int j = 0; j < spaceWidth; ++j)
                  {
                    std::cout << " ";
                  }
                if (breadthFirstFullTree_[index] != 0)
                  {
                    std::cout << std::setw(numWidth)
                              << breadthFirstFullTree_[index]->idx_;
                  }
                else
                  {
                    for (int j = 0; j < numWidth; ++j)
                      {
                        std::cout << " ";
                      }
                  }
                index++;
                std::cout.flags(original_flags);
                for (int j = 1; j < nodeNum; ++j)
                  {
                    for (int k = 0; k < interval; ++k)
                      {
                        for (int m = 0; m < (spaceWidth + numWidth); ++m)
                          {
                            std::cout << " ";
                          }
                      }
                    for (int k = 0; k < spaceWidth; ++k)
                      {
                        std::cout << " ";
                      }
                    if (breadthFirstFullTree_[index] != 0)
                      {
                        std::cout << std::setw(numWidth)
                                  << breadthFirstFullTree_[index]->idx_;
                      }
                    else
                      {
                        for (int j = 0; j < numWidth; ++j)
                          {
                            std::cout << " ";
                          }
                      }
                    index++;
                    std::cout.flags(original_flags);
                  }
                std::cout << std::endl;
                interval = start;
                start = (start - 1) / 2;
              }
          }
      }
  }

  void Read(std::istream& is)
  {
    size_t nodeNum = 0;
    std::vector<char> nodeTypes;
    readBasicType(is, verbose_);
    readBasicType(is, depth_);
    readBasicType(is, suspectLeaves_);
    readBasicType(is, nodeNum);

    nodeTypes.resize(nodeNum);
    for (index_t i = 0; i < nodeNum; ++i)
      {
        readBasicType(is, nodeTypes[i]);
      }

    size_t nodeNumBefore = nodes_.size();
    if (nodeNumBefore != 0)
      {
        for (index_t i = 0; i < nodeNumBefore; ++i)
          {
            delete nodes_[i];
          }
      }
    nodes_.resize(nodeNum);
    for (index_t i = 0; i < nodeNum; ++i)
      {
        if (nodeTypes[i] == 'l')
          {
            nodes_[i] = (Node*) new LeafT();
          }
        else
          {
            if (verbose_ == true)
              {
                nodes_[i] = (Node*) new SplitFullT();
              }
            else
              {
                nodes_[i] = (Node*) new SplitT();
              }
          }
        nodes_[i]->Read(is);
      }
    for (index_t i = 0; i < nodeNum; ++i)
      {
        if (nodeTypes[i] == 'l')
          {
            nodes_[i]->parent_ = nodes_[nodes_[i]->parentIdx_];
          }
        else
          {
            if (nodeTypes[i] == 'r')
              {
                nodes_[i]->parent_ = 0;
              }
            else
              {
                nodes_[i]->parent_ = nodes_[nodes_[i]->parentIdx_];
              }
            ((SplitNode*)nodes_[i])->leftChild_ =
                nodes_[((SplitNode*)nodes_[i])->leftChildIdx_];
            ((SplitNode*)nodes_[i])->rightChild_ =
                nodes_[((SplitNode*)nodes_[i])->rightChildIdx_];
          }
      }
  }

  void Write(std::ostream& os)
  {
    size_t nodeNum = nodes_.size();
    writeBasicType(os, verbose_);
    writeBasicType(os, depth_);
    writeBasicType(os, suspectLeaves_);
    writeBasicType(os, nodeNum);
    for (index_t i = 0; i < nodeNum; ++i)
      {
        writeBasicType(os, nodes_[i]->type_);
      }
    for (index_t i = 0; i < nodeNum; ++i)
      {
        nodes_[i]->Write(os);
      }
  }

  std::vector<Node*> nodes_;  // stored as depth first way

  // full binary tree stored as breadth first way, lacked nodes suppled with null pointer
  std::vector<Node*> breadthFirstFullTree_;

//  // stored as breath first way
//  std::vector<Node*> nodesBreadthFirst_;

  size_t depth_;
  index_t idx_;

  // store the number of leaf node with entropy higher
  // than user defined leafEntropy, could be considered as tree quality criterion
  size_t suspectLeaves_;

  bool verbose_;
};

#endif // TREE_H
