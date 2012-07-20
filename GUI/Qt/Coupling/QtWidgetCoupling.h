/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef QTWIDGETCOUPLING_H
#define QTWIDGETCOUPLING_H

#include <QWidget>
#include <PropertyModel.h>
#include <SNAPCommon.h>
#include <LatentITKEventNotifier.h>

/**
  Abstract class for hierarchy of data mappings between widgets and models. The mapping
  is created when a model is coupled to a widget. It handles mapping data from the model
  to the widget (when the model executes an appropriate event) and from the widget to the
  model. This abstract interface is generic and does not assume anything about the 
  model or the widget. 
  */
class AbstractWidgetDataMapping
{
public:
  virtual ~AbstractWidgetDataMapping() {}

  // Called the first time the widget is coupled to the model
  virtual void InitializeWidgetFromModel() = 0;

  // Called when the widget is changed
  virtual void UpdateModelFromWidget() = 0;

  // Called when the model is changed (event bucket contains the events that
  // have occured to the model, so we know what aspects of the widget need to
  // be updated)
  virtual void UpdateWidgetFromModel(const EventBucket &bucket) = 0;
};


/**
  Data mapping between a property model (encapsulating a value and a domain) 
  and an input widget of class TWidget. The behavior of this class is controlled
  by the template parameters WidgetValueTraits and WidgetDomainTraits, which 
  describe how values and domains are mapped to the specific widget. The traits
  classes for common Qt widgets are in header files QtXXXCoupling.h
  */
template <class TModel, class TWidgetPtr,
          class WidgetValueTraits, class WidgetDomainTraits>
class PropertyModelToWidgetDataMapping : public AbstractWidgetDataMapping
{
public:

  typedef TModel ModelType;
  typedef typename ModelType::ValueType AtomicType;
  typedef typename ModelType::DomainType DomainType;

  /** 
   * Constructor: set all the parameters of the mapping
   */
  PropertyModelToWidgetDataMapping(
      TWidgetPtr w, ModelType *model,
      WidgetValueTraits valueTraits, WidgetDomainTraits domainTraits)
    : m_Widget(w), m_Model(model), m_Updating(false),
      m_ValueTraits(valueTraits), m_DomainTraits(domainTraits) {}


  /** 
   * Called the first time the widget is coupled to the model in order to
   * populate the widget from the model.
   */
  void InitializeWidgetFromModel()
  {
    // Call the update method, including updating the domain
    this->DoUpdateWidgetFromModel(true, false);
  }

  /** 
   * Respond to a change in the model
   */
  void UpdateWidgetFromModel(const EventBucket &bucket)
  {
    // The bucket parameter tells us whether the domain changed or not
    this->DoUpdateWidgetFromModel(
          bucket.HasEvent(DomainChangedEvent()),
          bucket.HasEvent(DomainDescriptionChangedEvent()));
  }

  /**
   * Respond to the user changing the value in the widget
   */
  void UpdateModelFromWidget()
  {
    if(!m_Updating)
      {
      AtomicType user_value = m_ValueTraits.GetValue(m_Widget);
      AtomicType model_value;

      // Note: if the model reports that the value is invalid, we are not
      // allowing the user to mess with the value. This may have some odd
      // consequences. We need to investigate.
      if(m_Model->GetValueAndDomain(model_value, NULL) &&
         model_value != user_value)
        {
        m_Model->SetValue(user_value);
        }
      }
  }

protected:

  void DoUpdateWidgetFromModel(bool flagDomainChange, bool flagDomainDescriptionChange)
  {
    m_Updating = true;

    // The value is always needed
    AtomicType value;

    // The domain should only be updated in the bucket contains the range
    // event (the target range has been modified)
    if(flagDomainChange || flagDomainDescriptionChange)
      {
      // Prepopulate the range with current values in case the model does
      // not actually compute ranges
      DomainType domain = m_DomainTraits.GetDomain(m_Widget);

      // Obtain the value from the model
      if(m_Model->GetValueAndDomain(value, &domain))
        {
        if(flagDomainChange)
          {
          m_DomainTraits.SetDomain(m_Widget, domain);
          }
        else
          {
          m_DomainTraits.UpdateDomainDescription(m_Widget, domain);
          }

        m_ValueTraits.SetValue(m_Widget, value);
        }
      else
        {
        m_ValueTraits.SetValueToNull(m_Widget);
        }
      }

    else
      {
      // Domain has not changed, so we don't need to change it
      if(m_Model->GetValueAndDomain(value, NULL))
        {
        m_ValueTraits.SetValue(m_Widget, value);
        }
      else
        {
        m_ValueTraits.SetValueToNull(m_Widget);
        }
      }

    m_Updating = false;

  }

private:

  TWidgetPtr m_Widget;
  ModelType *m_Model;
  bool m_Updating;
  WidgetValueTraits m_ValueTraits;
  WidgetDomainTraits m_DomainTraits;
};


/**
  Base class for widget value traits. It specifies the methods that the traits
  classes must provide. See the QtXXXCoupling.h files for concrete implementations.
  */
template <class TAtomic, class TWidgetPtr>
class WidgetValueTraitsBase
{
public:
  // Get the Qt signal that the widget fires when its value has changed
  virtual const char *GetSignal() { return NULL; }

  // Get the value from the widget
  virtual TAtomic GetValue(TWidgetPtr) = 0;

  // Set the value of the widget
  virtual void SetValue(TWidgetPtr, const TAtomic &) = 0;

  // The default behavior for setting the widget to null is to do nothing.
  // This should be overridden by child traits classes
  virtual void SetValueToNull(TWidgetPtr) {}
};

/**
  Base class for widget domain traits. It specifies the methods that the traits
  classes must provide. See the QtXXXCoupling.h files for concrete implementations.
  */
template <class TDomain, class TWidgetPtr>
class WidgetDomainTraitsBase
{
public:
  /**
    Set the domain from the values in the model
   */
  virtual void SetDomain(TWidgetPtr w, const TDomain &domain) = 0;

  /**
    Update the description of the domain from the values in the model, without
    changing the configuration of the domain. This is only relevant for some
    kinds of domains, such as lists of items. In this case, we need to change
    the presentation of the items to the user, while keeping the list of items
    intact. By default, this does nothing.
    */
  virtual void UpdateDomainDescription(TWidgetPtr w, const TDomain &domain) {}

  /**
    Get the current value of the domain from the model. This is only
    needed in cases that the model does not provide full domain information
    (some information is coded in the widget by the developer). For widgets
    where the model fully specifies the domain, it's safe to just return
    a trivially initialized domain object.
    */
  virtual TDomain GetDomain(TWidgetPtr w) = 0;
};

/** Empty template for default value traits. Specialize for different Qt widgets */
template <class TAtomic, class TWidget>
class DefaultWidgetValueTraits {};

/** Empty template for default domain traits. Specialize for different Qt widgets */
template <class TDomain, class TWidget>
class DefaultWidgetDomainTraits {};

/** Default domain traits for models with a TrivialDomain. */
template <class TWidget>
class DefaultWidgetDomainTraits<TrivialDomain, TWidget>
    : public WidgetDomainTraitsBase<TrivialDomain, TWidget *>
{
public:
  // With a trivial domain, do nothing!
  virtual void SetDomain(TWidget *w, const TrivialDomain &) {}

  virtual TrivialDomain GetDomain(TWidget *w)
    { return TrivialDomain(); }
};

/** 
  Domain traits for widgets like combo boxes, list views and tables, which
  present the user with a list of options, from which a single choice is 
  selected. The actual widget-specific logic is handled by the TRowTraits
  template parameter, which defines how an option description provided by
  the model (TDesc) is appended as a row in the widget. TRowTraits should
  provide static methods appendRow and removeAll.

  The TItemDomain parameter needs to be a subclass of AbstractItemSetDomain
  or provide the same signature.
 */
template <class TItemDomain, class TWidget, class TRowTraits>
class ItemSetWidgetDomainTraits :
    public WidgetDomainTraitsBase<TItemDomain, TWidget *>
{
public:

  // The information about the item type are taken from the domain
  typedef typename TItemDomain::ValueType AtomicType;
  typedef typename TItemDomain::DescriptorType DescriptorType;
  typedef TItemDomain DomainType;

  void SetDomain(TWidget *w, const DomainType &domain)
  {
    // Remove all items from the widget
    TRowTraits::removeAll(w);

    // This is where we actually populate the widget
    for(typename DomainType::const_iterator it = domain.begin();
        it != domain.end(); it++)
      {
      // Get the key/value pair
      AtomicType value = domain.GetValue(it);
      const DescriptorType &row = domain.GetDescription(it);

      // Use the row traits to map information to the widget
      TRowTraits::appendRow(w, value, row);
      }
  }

  void UpdateDomainDescription(TWidget *w, const DomainType &domain)
  {
    // This is not the most efficient way of doing things, because we
    // are still linearly parsing through the widget and updating rows.
    // But at least the actual modifications to the widget are limited
    // to the rows that have been modified.
    //
    // What would be more efficient is to have a list of ids which have
    // been modified and update only those. Or even better, implement all
    // of this using an AbstractItemModel
    int nrows = TRowTraits::getNumberOfRows(w);
    for(int i = 0; i < nrows; i++)
      {
      AtomicType id = TRowTraits::getValueInRow(w, i);
      typename DomainType::const_iterator it = domain.find(id);
      if(it != domain.end())
        {
        const DescriptorType &row = domain.GetDescription(it);
        TRowTraits::updateRowDescription(w, i, row);
        }
      }
  }



  TItemDomain GetDomain(TWidget *w)
  {
    // We don't actually pull the widget because the domain is fully specified
    // by the model.
    return DomainType();
  }
};

/**
 * This Qt object is used to maintain the coupling between a model and a widget. It is 
 * memory managed by Qt and attached to the Qt widget that is involved in the coupling.
 */
class QtCouplingHelper : public QObject
{
  Q_OBJECT

public:
  explicit QtCouplingHelper(QWidget *widget, AbstractWidgetDataMapping *dm)
    : QObject(widget)
  {
    m_DataMapping = dm;
    setObjectName(QString("CouplingHelper:%1").arg(widget->objectName()));
  }

public slots:
  void onUserModification()
  {
    m_DataMapping->UpdateModelFromWidget();
  }

  void onPropertyModification(const EventBucket &bucket)
  {
    m_DataMapping->UpdateWidgetFromModel(bucket);
  }

protected:
  AbstractWidgetDataMapping *m_DataMapping;
};

/**
 * This function is the entry point into the coupling system between property models
 * (values with a domain) and Qt widgets. There are three versions of this method. 
 * All versions take a model and a widget as parameters. They differ in whether the
 * user explicitly specifies the traits objects for the values and domains, or whether
 * the default trait objects are used. 
 *
 * The last optional parameter allows the user to override the signal specified
 * in the default value traits for the widget. This is the signal in response to
 * which the model is updated from the widget
 */ 
template <class TModel, class TWidget,
          class WidgetValueTraits, class WidgetDomainTraits>
void makeCoupling(
    TWidget *w,
    TModel *model,
    WidgetValueTraits valueTraits,
    WidgetDomainTraits domainTraits,
    const char *signal = NULL)
{
  typedef typename TModel::ValueType ValueType;
  typedef typename TModel::DomainType DomainType;

  typedef PropertyModelToWidgetDataMapping<
      TModel, TWidget *,
      WidgetValueTraits, WidgetDomainTraits> MappingType;

  MappingType *mapping = new MappingType(w, model, valueTraits, domainTraits);

  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // Populate the widget (force the domain and value to be copied)
  mapping->InitializeWidgetFromModel();

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model, DomainChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model, DomainDescriptionChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  // Listen to value change events for this widget
  const char *mysignal = (signal) ? signal : valueTraits.GetSignal();
  h->connect(w, mysignal, SLOT(onUserModification()));
}

template <class TModel, class TWidget, class WidgetValueTraits>
void makeCoupling(TWidget *w,
                  TModel *model,
                  WidgetValueTraits trValue,
                  const char *signal = NULL)
{
  typedef typename TModel::DomainType DomainType;
  typedef DefaultWidgetDomainTraits<DomainType, TWidget> WidgetDomainTraits;
  makeCoupling<TModel, TWidget,
      WidgetValueTraits, WidgetDomainTraits>(
        w, model, trValue, WidgetDomainTraits(), signal);
}


/**
  Create a coupling between a numeric model and a Qt widget. The widget
  will listen to the events from the model and update its value and range
  accordingly. When the user interacts with the widget, the model will be
  updated. The coupling fully automates mapping of data between Qt input
  widgets and SNAP numeric models.

  This version of the method uses default traits. There is also a version
  that allows you to provide your own traits.
*/
template <class TModel, class TWidget>
void makeCoupling(TWidget *w,
                  TModel *model,
                  const char *signal = NULL)
{
  typedef typename TModel::ValueType ValueType;
  typedef DefaultWidgetValueTraits<ValueType, TWidget> WidgetValueTraits;
  makeCoupling<TModel, TWidget,WidgetValueTraits>(
        w, model, WidgetValueTraits(), signal);
}


#endif // QTWIDGETCOUPLING_H
