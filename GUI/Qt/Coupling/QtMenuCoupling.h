#ifndef QTMENUCOUPLING_H
#define QTMENUCOUPLING_H

#include <QMenu>
#include <QtWidgetCoupling.h>

template <class TAtomic>
class QActionTimestamper : public QObject
{
public:
  typedef std::pair<unsigned int, TAtomic> StoredData;
  QActionTimestamper(QObject *parent) : QObject(parent) {}
public slots:
  void triggered(QAction *action) {
    static unsigned int t_stamp_global = 0;
    unsigned int t_stamp;
    TAtomic value;
    std::tie(t_stamp, value) = action->data().value<StoredData>();
    action->setData(std::make_pair(t_stamp_global++, value));
  }
protected:
};


/**
  These are the row traits for adding and updating rows in menus attached to buttons.
  This class is further parameterized by the class TItemDesriptionTraits, which is
  used to obtain the text and icon information from the value/description pairs
  provided by the model.
  */
template <class TAtomic>
class SimpleMenuRowTraits
{
public:
  typedef std::pair<unsigned int, TAtomic> StoredData;

  static void removeAll(QMenu *w)
  {
    w->clear();
  }

  static int getNumberOfRows(QMenu *w)
  {
    return w->actions().count();
  }

  static TAtomic getValueInRow(QMenu *w, int i)
  {
    unsigned int t_stamp;
    TAtomic value;
    std::tie(t_stamp, value) = w->actions().at(i)->data().value<StoredData>();
    return value;
  }

  static void appendRow(QMenu *w, TAtomic label, const std::string &desc)
  {
    // The description
    QString text = QString::fromStdString(desc);

    // Create an action
    QAction *action = new QAction(w);
    action->setData(QVariant::fromValue(std::make_pair(0u, label)));
    action->setText(text);
    w->addAction(action);

    // We need to keep track of the action firing
    auto *ts = new QActionTimestamper<TAtomic>(action);
    QObject::connect(w, SIGNAL(triggered(QAction*)), ts, SLOT(triggered(QAction*)), Qt::DirectConnection);
  }

  static void updateRowDescription(QMenu *w, int index, const std::string &desc)
  {
    // The current value
    QAction *action = w->actions().at(index);
    auto sd = action->data().value<StoredData>();

    // Get the properies and compare them to the color label
    QString currentText = action->text();
    QString newText = QString::fromStdString(desc);
    if(currentText != newText)
      action->setText(newText);
  }
};

// Define the defaults
template <class TDomain>
class SimpleMenuDomainTraits
  : public ItemSetWidgetDomainTraits<
      TDomain, QMenu,
      SimpleMenuRowTraits<typename TDomain::ValueType> >
{
};



/**
 * A coupling between an atomic class that represents different values and
 * a menu
 */
template <typename TAtomic>
class MenuValueTraits
  : public WidgetValueTraitsBase<TAtomic, QMenu *>
{
public:
  typedef std::pair<unsigned int, TAtomic> StoredData;

  const char *GetSignal()
  {
    return SIGNAL(triggered(QAction *));
  }

  TAtomic GetValue(QMenu *w)
  {
    TAtomic last_value;
    unsigned int t_last = 0;
    for(auto &a : w->actions())
    {
      unsigned int t_stamp;
      TAtomic value;
      std::tie(t_stamp, value) = a->data().value<StoredData>();
      if(t_stamp > t_last)
        last_value = value;
    }
    return last_value;
  }

  void SetValue(QMenu *w, const TAtomic &value)
  {
    if(w->parentWidget())
      w->parentWidget()->setEnabled(true);
  }

  void SetValueToNull(QMenu *w)
  {
    if(w->parentWidget())
      w->parentWidget()->setDisabled(true);
  }
};

#endif // QTMENUCOUPLING_H
