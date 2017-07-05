#ifndef COLORLABELPROPERTYMODEL_H
#define COLORLABELPROPERTYMODEL_H

#include <PropertyModel.h>
#include <ColorLabel.h>
#include <ColorLabelTable.h>

/*
  This module contains property models having to do with selection of
  color labels from the color label table.
  */

// Define the domain that wraps around a std::map of color labels
typedef STLMapWrapperItemSetDomain<LabelType, ColorLabel> ColorLabelItemSetDomain;

// Base class typedef for the concrete implementation below
typedef ConcretePropertyModel<LabelType, ColorLabelItemSetDomain>
  ConcreteColorLabelPropertyModelBase;

/**
  This is an ITK-SNAP model that internally stores a color label and provides
  a set of options, correponding to currently available color labels
  */
class ConcreteColorLabelPropertyModel
    : public ConcretePropertyModel<LabelType, ColorLabelItemSetDomain>
{
public:
  // Standard ITK stuff
  irisITKObjectMacro(ConcreteColorLabelPropertyModel,
                     ConcreteColorLabelPropertyModelBase)

  // Domain typedef
  typedef ColorLabelItemSetDomain DomainType;

  /** Set the color label table, from which this model constructs its domain
    representation */
  void Initialize(ColorLabelTable *clt);
};

/**
  This iterator presents all the draw-over options in a linear fashion.
  It first go through the wildcard coverage modes (PAINT_OVER_ALL,
  PAINT_OVER_VISIBLE) and then throught the valid labels
  */
class DrawOverLabelItemSetIterator
{
public:
  typedef DrawOverLabelItemSetIterator Self;

  Self& operator ++();

  bool operator == (const Self &ref);
  bool operator != (const Self &ref);

  friend class DrawOverLabelItemSetDomain;

private:
  DrawOverLabelItemSetIterator(ColorLabelTable *table);
  CoverageModeType m_PointedMode;
  ColorLabelTable::ValidLabelConstIterator m_LabelIter;
};

// A custom domain for working with draw-over compound
class DrawOverLabelItemSetDomain : public AbstractItemSetDomain<
    DrawOverFilter, ColorLabel, DrawOverLabelItemSetIterator>
{
public:
  typedef DrawOverLabelItemSetIterator const_iterator;

  DrawOverLabelItemSetDomain();
  virtual ~DrawOverLabelItemSetDomain() {}

  void Initialize(ColorLabelTable *clt);

  virtual const_iterator begin() const;
  virtual const_iterator end() const;
  virtual const_iterator find(const DrawOverFilter &value) const;
  virtual DrawOverFilter GetValue(const const_iterator &it) const;
  virtual ColorLabel GetDescription(const const_iterator &it) const;

  bool operator == (const DrawOverLabelItemSetDomain &refdom);
  bool operator != (const DrawOverLabelItemSetDomain &refdom);

  bool isAtomic() { return false; }

private:

  ColorLabelTable *m_LabelTable;
};

// Base class for the following model
typedef ConcretePropertyModel<DrawOverFilter, DrawOverLabelItemSetDomain>
  ConcreteDrawOverFilterPropertyModelBase;

/**
  This is an ITK-SNAP model that internally stores a draw-over state atomic
  object.
  */
class ConcreteDrawOverFilterPropertyModel
    : public ConcreteDrawOverFilterPropertyModelBase
{
public:
  // Standard ITK stuff
  irisITKObjectMacro(ConcreteDrawOverFilterPropertyModel,
                     ConcreteDrawOverFilterPropertyModelBase)

  // Domain typedef
  typedef DrawOverLabelItemSetDomain DomainType;

  /** Set the color label table, from which this model constructs its domain
    representation */
  void Initialize(ColorLabelTable *clt);
};



#endif // COLORLABELPROPERTYMODEL_H
