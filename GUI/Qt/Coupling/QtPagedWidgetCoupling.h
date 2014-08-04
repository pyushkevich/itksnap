#ifndef QTPAGEDWIDGETCOUPLING_H
#define QTPAGEDWIDGETCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QTabWidget>
#include <QStackedWidget>
#include <map>

/**
 * Coupling for widgets such as Stack, Tab, etc. Each value of the
 * coupled variable corresponds to a page in the widget
 */

template <class TAtomic, class TPagedWidget>
struct PagedWidgetValueTraits :
    public WidgetValueTraitsBase<TAtomic, TPagedWidget *>
{
public:
  // There needs to be a map from the values of the atomic variable to
  // the pages in the widget
  typedef std::map<TAtomic, QWidget *> PageMap;
  typedef typename PageMap::iterator PageIterator;

  PagedWidgetValueTraits(PageMap pm) : m_PageMap(pm) {}

  // The signal fired when the active page is changed
  virtual const char *GetSignal()
  {
    return SIGNAL(currentChanged(int));
  }


  TAtomic GetValue(TPagedWidget *w)
  {
    // Figure out which button is checked
    for(PageIterator it = m_PageMap.begin(); it != m_PageMap.end(); ++it)
      {
      if(it->second == w->currentWidget())
        return it->first;
      }

    // This is ambiguous...
    return static_cast<TAtomic>(0);
  }

  void SetValue(TPagedWidget *w, const TAtomic &value)
  {
    // Set all the buttons
    QWidget *page = m_PageMap[value];
    w->setCurrentWidget(page);
  }

  void SetValueToNull(TPagedWidget *w)
  {
    // Set all the buttons
    w->setCurrentIndex(-1);
  }

protected:

  PageMap m_PageMap;
};

template <class TDomain, class TPagedWidget>
class DefaultPagedWidgetDomainTraits
    : public WidgetDomainTraitsBase<TDomain, TPagedWidget *>
{
public:
  // With a trivial domain, do nothing!
  virtual void SetDomain(TPagedWidget *w, const TDomain &) {}

  virtual TDomain GetDomain(TPagedWidget *w)
    { return TDomain(); }

};

template <class TDomain>
class DefaultWidgetDomainTraits<TDomain, QStackedWidget>
    : public DefaultPagedWidgetDomainTraits<TDomain, QStackedWidget>
{ };

template <class TDomain>
class DefaultWidgetDomainTraits<TDomain, QTabWidget>
    : public DefaultPagedWidgetDomainTraits<TDomain, QTabWidget>
{ };


template <class TModel, class TPageWidget>
void makePagedWidgetCoupling(
    TPageWidget *w,
    TModel *model,
    std::map<typename TModel::ValueType, QWidget *> pageMap,
    QtCouplingOptions opts = QtCouplingOptions())
{
  typedef typename TModel::ValueType ValueType;
  typedef PagedWidgetValueTraits<ValueType, TPageWidget> WidgetValueTraits;

  WidgetValueTraits traits(pageMap);

  makeCoupling<TModel, TPageWidget, WidgetValueTraits>(w, model, traits, opts);
}




#endif // QTPAGEDWIDGETCOUPLING_H
