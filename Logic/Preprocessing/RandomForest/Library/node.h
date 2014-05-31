/**
 * Define node inheritance:
 * DTLeaf::Node
 * DTSplitFull::DTSplit::SplitNode::Node
 *
 * and its constructor, streaming and serialization interface.
 */


#ifndef NODE_H
#define NODE_H

#include "utility.h"

class Node
{
public:
  Node(): type_('l'), parent_(0) {}
  Node(char type): type_(type), parent_(0) {}
  Node(Node* parent): type_('l'), parent_(parent) {}
  Node(char type, Node* parent): type_(type), parent_(parent) {}

  bool IsRoot()
  {
    if (type_ == 'r')
      {
        return true;
      }
    else
      {
        return false;
      }
  }

  bool IsLeaf()
  {
    if (type_ == 'l')
      {
        return true;
      }
    else
      {
        return false;
      }
  }

  bool IsSplit()
  {
    if (type_ == 's')
      {
        return true;
      }
    else
      {
        return false;
      }
  }

  // four digits indicate print results
  // 0 : nothing
  // 0001: node index, 0002: node index + address, 0003: node address only
  // 0010: statistics, 0020: statistics + address
  // 0100: classifier, 0200: classifier + address
  // 1000: tree structure depth first, 2000: breath first
  virtual void Print(int level)
  {
    if (((level - (level/10)*10)  == 1) ||
        ((level - (level/10)*10)  == 2) ||
        ((level - (level/10)*10)  == 3))
      {
        std::cout << "* Node: ";
        if (IsRoot())
          {
            std::cout << "root ";
          }
        else if (IsLeaf())
          {
            std::cout << "leaf ";
          }
        else
          {
            std::cout << "split";
          }
        if (((level - (level/10)*10)  == 1) ||
            ((level - (level/10)*10)  == 2))
          {
            std::cout << "    (Idx: " << idx_ << ")";
            if (!IsRoot())
              {
                std::cout << "    (Parent Idx: " << parentIdx_ << ")";
              }
          }
        if (((level - (level/10)*10)  == 2) ||
            ((level - (level/10)*10)  == 3))
          {
            std::cout << "    [Addr: " << this << "]";
            if (!IsRoot())
              {
                std::cout << "    [Parent Addr: " << parent_ << "]";
              }
          }
        std::cout << std::endl;
      }
  }

  virtual void Read(std::istream& is)
  {
    readBasicType(is, type_);
    readBasicType(is, idx_);
    readBasicType(is, parentIdx_);
  }

  virtual void Write(std::ostream& os)
  {
    writeBasicType(os, type_);
    writeBasicType(os, idx_);
    writeBasicType(os, parentIdx_);
  }

  // node type (r:root, l:leaf, s:split)
  char type_;
  index_t idx_;
  index_t parentIdx_;
  Node* parent_;
};

class SplitNode : public Node
{
public:
  SplitNode(): Node('s'), leftChild_(0), rightChild_(0) {}
  SplitNode(Node* parent): Node('s', parent) {}
  SplitNode(Node* parent, Node* leftChild, Node* rightChild)
    : Node('s', parent), leftChild_(leftChild), rightChild_(rightChild) {}

  virtual void Print(int level)
  {
    Node::Print(level);
    if (((level - (level/10)*10)  == 1) ||
        ((level - (level/10)*10)  == 2) ||
        ((level - (level/10)*10)  == 3))
      {
        if (((level - (level/10)*10)  == 1) ||
            ((level - (level/10)*10)  == 2))
          {
            std::cout << "    (LeftChild Idx: " << leftChildIdx_ << ")"
                      << "    (RightChild Idx: " << rightChildIdx_ << ")";
          }
        if (((level - (level/10)*10)  == 2) ||
            ((level - (level/10)*10)  == 3))
          {
            std::cout << "    [LeftChild Addr: " << leftChild_ << "]"
                      << "    [RightChild Addr: " << rightChild_ << "]";
          }
        std::cout << std::endl;
      }
  }

  virtual void Read(std::istream& is)
  {
    Node::Read(is);
    readBasicType(is, leftChildIdx_);
    readBasicType(is, rightChildIdx_);
  }

  virtual void Write(std::ostream& os)
  {
    Node::Write(os);
    writeBasicType(os, leftChildIdx_);
    writeBasicType(os, rightChildIdx_);
  }

  index_t leftChildIdx_;
  index_t rightChildIdx_;
  Node* leftChild_;
  Node* rightChild_;
};

template<class S>
class DTLeaf : public Node
{
public:
  DTLeaf(): Node() {}
  DTLeaf(Node* parent): Node(parent) {}
  DTLeaf(S& statistics): Node(), statistics_(statistics) {} //Need to define copy constructor in S
  DTLeaf(Node* parent, S& statistics): Node(parent), statistics_(statistics) {}
  virtual void Print(int level)
  {
    Node::Print(level);
    if (((level/10 - (level/100)*10)  == 1) ||
        ((level/10 - (level/100)*10)  == 2))
      {
        statistics_.Print(level);
      }
    std::cout << std::endl;
  }

  virtual void Read(std::istream& is)
  {
    Node::Read(is);
    statistics_.Read(is);
  }

  virtual void Write(std::ostream& os)
  {
    Node::Write(os);
    statistics_.Write(os);
  }

  S statistics_;
  double informationGain_;
};

template<class C>
class DTSplit : public SplitNode
{
public:
  DTSplit(): SplitNode() {}
  DTSplit(Node* parent): SplitNode(parent) {}
  DTSplit(Node* parent, Node* leftChild, Node* rightChild)
    : SplitNode(parent, leftChild, rightChild) {}
  DTSplit(C& classifier)
    : SplitNode(), classifier_(classifier) {}//Need to define copy constructor in C
  DTSplit(Node* parent, C& classifier)
    : SplitNode(parent), classifier_(classifier) {}
  DTSplit(Node* parent, Node* leftChild, Node* rightChild, C& classifier)
    : SplitNode(parent, leftChild, rightChild), classifier_(classifier) {}
  virtual void Print(int level)
  {
    SplitNode::Print(level);
    if (((level/100 - (level/1000)*10)  == 1) ||
        ((level/100 - (level/1000)*10)  == 2))
      {
        classifier_.Print(level);
      }
    std::cout << std::endl;
  }

  virtual void Read(std::istream& is)
  {
    SplitNode::Read(is);
    classifier_.Read(is);
  }

  virtual void Write(std::ostream& os)
  {
    SplitNode::Write(os);
    classifier_.Write(os);
  }

  C classifier_;
  double informationGain_;
};

template<class S, class C>
class DTSplitFull : public DTSplit<C>
{
public:
  DTSplitFull(): DTSplit<C>() {}
  DTSplitFull(Node* parent): DTSplit<C>(parent) {}
  DTSplitFull(Node* parent, Node* leftChild, Node* rightChild)
    : DTSplit<C>(parent, leftChild, rightChild) {}
  DTSplitFull(S& statistics, C& classifier)
    : DTSplit<C>(classifier), statistics_(statistics) {}
  DTSplitFull(Node* parent, S& statistics, C& classifier)
    : DTSplit<C>(parent, classifier), statistics_(statistics) {}
  DTSplitFull(Node* parent, Node* leftChild, Node* rightChild,
              S& statistics, C& classifier)
    : DTSplit<C>(parent, leftChild, rightChild, classifier),
      statistics_(statistics) {}

  virtual void Print(int level)
  {
    SplitNode::Print(level);
    if (((level/10 - (level/100)*10)  == 1) ||
        ((level/10 - (level/100)*10)  == 2))
      {
        statistics_.Print(level);
      }
    if (((level/100 - (level/1000)*10)  == 1) ||
        ((level/100 - (level/1000)*10)  == 2))
      {
        DTSplitFull<S,C>::classifier_.Print(level);
      }
    std::cout << std::endl;
  }

  virtual void Read(std::istream& is)
  {
    DTSplit<C>::Read(is);
    statistics_.Read(is);
  }

  virtual void Write(std::ostream& os)
  {
    DTSplit<C>::Write(os);
    statistics_.Write(os);
  }

  S statistics_;
};

#endif // NODE_H
