#ifndef SNAPUIFLAG_H
#define SNAPUIFLAG_H

#include "StateManagement.h"
#include "UIState.h"

class GlobalUIModel;

/**
  A BooleanCondition implementation that queries the GlobalUIModel
  about different UI states.
  */
class SNAPUIFlag : public BooleanCondition
{
public:
  typedef SNAPUIFlag Self;
  typedef BooleanCondition Superclass;
  itkTypeMacro(Self, Superclass)

  static SmartPtr<Self> New(GlobalUIModel *model, UIState state);

  bool operator() () const;

protected:

  SNAPUIFlag(GlobalUIModel *model, UIState state);
  virtual ~SNAPUIFlag() {}

private:
  GlobalUIModel *m_Model;
  UIState m_State;
};

#endif // SNAPUIFLAG_H
