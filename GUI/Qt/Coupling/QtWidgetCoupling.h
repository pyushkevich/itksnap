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
#include <QVariant>
#include <QAction>
#include <PropertyModel.h>
#include <SNAPCommon.h>
#include <LatentITKEventNotifier.h>
#include "QtWidgetActivator.h"

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
      m_ValueTraits(valueTraits), m_DomainTraits(domainTraits),
      m_CachedValueAvailable(false),
      m_CachedDomainAvailable(false),
      m_AllowUpdateInInvalidState(false),
      m_LastBucketMTime(0) {}

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
   * Whether the user can update the model when it is in invalid state
   */
  irisSetMacro(AllowUpdateInInvalidState, bool)

  /** 
   * Respond to a change in the model
   */
  void UpdateWidgetFromModel(const EventBucket &bucket)
  {
    // Make sure we don't process the same event bucket twice
    if(bucket.GetMTime() <= m_LastBucketMTime)
      return;

    // The bucket parameter tells us whether the domain changed or not
    this->DoUpdateWidgetFromModel(
          bucket.HasEvent(DomainChangedEvent()),
          bucket.HasEvent(DomainDescriptionChangedEvent()));

    // Store the bucket id
    m_LastBucketMTime = bucket.GetMTime();
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

      // Get the current value and status from the model
      // TODO: this information could be cached, and dirtied in the case of
      // an event. Currently, we are doing a lot of unnecessary accesses to
      // the model just to check hte validity and current value of th model
      bool valid = m_Model->GetValueAndDomain(model_value, NULL);

      // Note: if the model reports that the value is invalid, we are not
      // allowing the user to mess with the value. This may have some odd
      // consequences. We need to investigate.
      if((valid && model_value != user_value)
         || (!valid && m_AllowUpdateInInvalidState))
        {
        m_Model->SetValue(user_value);
        m_CachedWidgetValue = user_value;
        m_CachedValueAvailable = true;
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
    DomainType *domptr = NULL;
    if(flagDomainChange || flagDomainDescriptionChange)
      {
      m_DomainTemp = m_DomainTraits.GetDomain(m_Widget);
      domptr = &m_DomainTemp;
      }

    // Obtain the value from the model
    if(m_Model->GetValueAndDomain(value, domptr))
      {
      // Update the domain if necessary. The updates to the domain never come
      // in response to the user interaction with m_Widget, so it's safe to
      // just call SetDomain without first checking if the change is 'real'.
      if(flagDomainChange)
        {
        if(!m_CachedDomainAvailable || m_CachedWidgetDomain != m_DomainTemp)
          {
          m_DomainTraits.SetDomain(m_Widget, m_DomainTemp);

          // Once the domain changes, the current cached value can no longer be
          // trusted because the widget may have reconfigured.
          m_CachedValueAvailable = false;

          // Cache the domain if it is atomic (supports comparison operator)
          if(m_DomainTemp.isAtomic())
            {
            m_CachedWidgetDomain = m_DomainTemp;
            m_CachedDomainAvailable = true;
            }
          }
        }
      else if(flagDomainDescriptionChange)
        {
        m_DomainTraits.UpdateDomainDescription(m_Widget, m_DomainTemp);
        }

      // Before setting the value, we should check it against the cached value
      if(!m_CachedValueAvailable || m_CachedWidgetValue != value)
        {
        m_ValueTraits.SetValue(m_Widget, value);
        m_CachedWidgetValue = value;
        m_CachedValueAvailable = true;
        }
      }
    else
      {
      m_ValueTraits.SetValueToNull(m_Widget);
      m_CachedValueAvailable = false;
      }

    m_Updating = false;
  }

private:

  TWidgetPtr m_Widget;
  ModelType *m_Model;
  bool m_Updating;
  WidgetValueTraits m_ValueTraits;
  WidgetDomainTraits m_DomainTraits;

  // Whether the user can update the model when it is in invalid state
  bool m_AllowUpdateInInvalidState;

  // A specific property of the widget that should be toggled when the model
  // is in NULL state (visible/enabled)
  QString m_NullStateProperty;

  // A temporary copy of the domain
  DomainType m_DomainTemp;
  DomainType m_CachedWidgetDomain;

  // We cache the last known value in the widget to avoid unnecessary updates
  // of the widget in response to events generated from the model after the
  // model has been updated in response to the widget.
  AtomicType m_CachedWidgetValue;

  // Whether or not the cached value can be used
  bool m_CachedValueAvailable, m_CachedDomainAvailable;

  // The last bucked ID handled by this widget. This is used to protect against
  // updates being invoked twice with the same event bucket (seems like some
  // sort of a Qt weirdness)
  unsigned long m_LastBucketMTime;
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
  // For example { return SIGNAL(triggered()); }
  virtual const char *GetSignal() { return NULL; }

  // Get the widget that generates the signal above. Normally it's the same
  // as the input widget, but in some cases, it's some other QObject that
  // fires the signal, i.e., in QAbstractItemView, it would be the item
  // selection model rather than the view itself
  virtual QObject *GetSignalEmitter(QObject *w) { return w; }

  // Get the value from the widget
  virtual TAtomic GetValue(TWidgetPtr) = 0;

  // Set the value of the widget
  virtual void SetValue(TWidgetPtr, const TAtomic &) = 0;

  // The default behavior for setting the widget to null is to do nothing.
  // This should be overridden by child traits classes
  virtual void SetValueToNull(TWidgetPtr) {}

  virtual ~WidgetValueTraitsBase() {}
};


/**
 * Widget value traits that connect a boolean property of a Qt widget with
 * a boolean (or boolean-castable) value. These traits don't handle the
 * property having a null value - probably should be addressed in the future
 */
template <class TAtomic>
class WidgetBooleanNamedPropertyTraits
    : public WidgetValueTraitsBase<TAtomic, QObject *>
{
public:
  WidgetBooleanNamedPropertyTraits(const char *propertyName, bool reverse)
    : m_Property(propertyName), m_Reverse(reverse) {}

  virtual TAtomic GetValue(QObject *w)
  {
    if(m_Reverse)
      return !qvariant_cast<TAtomic>(w->property(m_Property.c_str()));
    else
      return qvariant_cast<TAtomic>(w->property(m_Property.c_str()));
  }
  virtual void SetValue(QObject *w, const TAtomic &value)
  {
    if(m_Reverse)
      w->setProperty(m_Property.c_str(), !value);
    else
      w->setProperty(m_Property.c_str(), value);
  }

protected:
  std::string m_Property;
  bool m_Reverse;
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

  virtual ~WidgetDomainTraitsBase() {}
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
 * Empty template for default domain traits for widgets that contain
 * multiple rows. Specialize for different Qt widgets. One way to specialize
 * is to derive from the class ItenSetWidgetDomainTraits below, although it
 * is not the only way to specialize.
 */
template <class TDomain, class TWidget, class TRowTraits>
class DefaultMultiRowWidgetDomainTraits {};

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
        it != domain.end(); ++it)
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
  explicit QtCouplingHelper(QObject *widget, AbstractWidgetDataMapping *dm)
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
  This class simplifies passing parameters to the makeCoupling functions.
  */
struct QtCouplingOptions
{
  enum Option
  {
    NO_OPTIONS = 0x0,

    /**
      This flag disconnects the Widget -> Model direction of the coupling. The
      coupling will update the widget value and domain in response to changes
      in the model, but it will not update the model from changes in the widget.
      This is useful when we want to explicitly handle changes in the
      widget. For example, if we need to check the validity of the value and/or
      present a dialog box, we would use this option, and then set up an explicit
      slot to handle changes from the widget and update the model.
      */
    UNIDIRECTIONAL = 0x1,

    /**
     * This flag makes it possible for the widget to write its value to the model
     * when the model is in invalid state. By default, this is disabled and any
     * changes by the user to a widget in invalid state are ignored, i.e. not
     * sent to the model.
     */
    ALLOW_UPDATES_WHEN_INVALID = 0x2,

    /**
     * Whether the widget should be deactivated when the model is in Null state.
     * Passing this option to makeCoupling will create a widget activator for the
     * widget. Make sure no other activators are set on the widget
     */
    DEACTIVATE_WHEN_INVALID = 0x4
  };

  Q_DECLARE_FLAGS(Options, Option)

  irisGetSetMacro(Options, Options)
  irisGetSetMacro(SignalOverride, const char *)

  bool IsUnidirectional()
    { return m_Options.testFlag(UNIDIRECTIONAL); }

  bool IsUpdateAllowedWhenInvalid()
    { return m_Options.testFlag(ALLOW_UPDATES_WHEN_INVALID); }

  bool IsDeactivatedWhenInvalid()
    { return m_Options.testFlag(DEACTIVATE_WHEN_INVALID); }

  QtCouplingOptions(Options opts = NO_OPTIONS)
    : m_Options(opts), m_SignalOverride(NULL) {}

  QtCouplingOptions(const char *signal, Options opts = NO_OPTIONS)
    : m_Options(opts), m_SignalOverride(signal)  {}

  QtCouplingOptions(const QtCouplingOptions &ref)
    : m_Options(ref.m_Options), m_SignalOverride(ref.m_SignalOverride) {}

private:

  /**
   * The options
   */
  Options m_Options;

  /**
    This value allow the default signal associated with a widget coupling
    to be overriden. For each Qt widget, there is a default signal that we
    have picked, but in some cases there may be a reason to choose a different
    signal. The model is updated in response to this signal. When set to NULL
    this means that the default signal will be used.
    */
  const char *m_SignalOverride;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QtCouplingOptions::Options)

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
    QtCouplingOptions opts = QtCouplingOptions())
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

  // Listen to change events on this widget, unless asked not to
  if(!opts.IsUnidirectional())
    {
    const char *mysignal = (opts.GetSignalOverride())
        ? opts.GetSignalOverride() : valueTraits.GetSignal();
    QObject *emitter = valueTraits.GetSignalEmitter(w);
    if(mysignal && emitter)
      h->connect(emitter, mysignal, SLOT(onUserModification()));
    }

  // Set the allow-changes flag in the mapping
  if(opts.IsUpdateAllowedWhenInvalid())
    {
    mapping->SetAllowUpdateInInvalidState(true);
    }

  // Create an activator
  if(opts.IsDeactivatedWhenInvalid())
    {
    activateOnFlag(w, model, UIF_PROPERTY_IS_VALID);
    }
}

template <class TModel, class TWidget, class WidgetValueTraits>
void makeCoupling(TWidget *w,
                  TModel *model,
                  WidgetValueTraits trValue,
                  QtCouplingOptions opts = QtCouplingOptions())
{
  typedef typename TModel::DomainType DomainType;
  typedef DefaultWidgetDomainTraits<DomainType, TWidget> WidgetDomainTraits;
  makeCoupling<TModel, TWidget,
      WidgetValueTraits, WidgetDomainTraits>(
        w, model, trValue, WidgetDomainTraits(), opts);
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
                  QtCouplingOptions opts = QtCouplingOptions())
{
  typedef typename TModel::ValueType ValueType;
  typedef DefaultWidgetValueTraits<ValueType, TWidget> WidgetValueTraits;
  makeCoupling<TModel, TWidget,WidgetValueTraits>(
        w, model, WidgetValueTraits(), opts);
}


/**
  Create a coupling between a PropertyModel with some kind of an item set domain
  and a Qt widget that holds multiple items or rows (QListView, QComboBox,
  QToolBar, etc.). Unlike simple kinds of couplings (QLineEdit, etc), these
  more complex couplings are parameterized by the type TRowTraits, which
  describe the mapping of each item in the PropertyModel to each row in the
  widget. This version of the coupling method takes TRowTraits as an extra
  parameter, and creates an appropriate domain
  */
template <class TModel, class TWidget, class TRowTraits>
void makeMultiRowCoupling(TWidget *w,
                          TModel *model,
                          TRowTraits rowTraits,
                          QtCouplingOptions opts = QtCouplingOptions())
{
  typedef typename TModel::ValueType ValueType;
  typedef DefaultWidgetValueTraits<ValueType, TWidget> WidgetValueTraits;

  typedef typename TModel::DomainType DomainType;
  typedef DefaultMultiRowWidgetDomainTraits<DomainType, TWidget, TRowTraits> WidgetDomainTraits;

  makeCoupling<TModel, TWidget, WidgetValueTraits, WidgetDomainTraits>(
        w, model, WidgetValueTraits(), WidgetDomainTraits(), opts);
}

/**
 * Create a coupling between a boolean (or integer) model and a named property
 * in a Qt widget, such as "visible". This allows us to hide/show and otherwise
 * modify widgets in response to changes in the model layer.
 */
template <class TModel>
void makeBooleanNamedPropertyCoupling(
    QObject *w, const char *propertyName,
    TModel *model, bool reverse = false,
    QtCouplingOptions opts = QtCouplingOptions())
{
  // Create the traits for the property
  typedef typename TModel::ValueType ValueType;
  typedef WidgetBooleanNamedPropertyTraits<ValueType> TraitsType;
  TraitsType traits(propertyName, reverse);

  // Call the main coupling method
  makeCoupling<TModel, QObject, TraitsType>(w, model, traits, opts);
}

/**
 * Couple the visibility of a widget to a boolean or integer-valued model
 */
template <class TModel>
void makeWidgetVisibilityCoupling(
    QWidget *w, TModel *model, bool reverse = false,
    QtCouplingOptions opts = QtCouplingOptions())
{
  makeBooleanNamedPropertyCoupling(w, "visible", model, reverse, opts);
}


/**
 * Couple the visibility of a widget to a boolean or integer-valued model
 */
template <class TModel>
void makeActionVisibilityCoupling(
    QAction *w, TModel *model, bool reverse = false,
    QtCouplingOptions opts = QtCouplingOptions())
{
  makeBooleanNamedPropertyCoupling(w, "visible", model, reverse, opts);
}

#endif // QTWIDGETCOUPLING_H
