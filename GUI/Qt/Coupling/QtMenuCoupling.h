#ifndef QTMENUCOUPLING_H
#define QTMENUCOUPLING_H

#include <QMenu>
#include <QActionGroup>
#include <QtWidgetCoupling.h>

/**
  Default traits for mapping a numeric value (or any sort of key, actually)
  to a row in a combo box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QMenu>
  : public WidgetValueTraitsBase<TAtomic, QMenu *>
{
public:

  // In addition to the value, action's data will store an icon signature
  using ValueIconSigPair = QPair<TAtomic, QVariant>;

  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(triggered(QAction *));
  }

  TAtomic GetValue(QMenu *w)
  {
    auto *ag = w->actions().size() ? w->actions().first()->actionGroup() : nullptr;
    if (ag)
      return ag->checkedAction()->data().value<ValueIconSigPair>().first;

    return QVariant().value<TAtomic>();
  }

  void SetValue(QMenu *w, const TAtomic &value)
  {
    auto *ag = w->actions().size() ? w->actions().first()->actionGroup() : nullptr;
    if(ag)
      for(auto *action : ag->actions())
      {
        if(action->data().value<ValueIconSigPair>().first == value)
          action->setChecked(true);
      }
  }

  void SetValueToNull(QMenu *w)
  {
    auto *ag = w->actions().size() ? w->actions().first()->actionGroup() : nullptr;
    if(ag)
      for(auto *action : ag->actions())
      {
        action->setChecked(false);
      }
  }
};


/**
  These are the row traits for adding and updating rows in menus attached to buttons.
  This class is further parameterized by the class TItemDesriptionTraits, which is
  used to obtain the text and icon information from the value/description pairs
  provided by the model.
  */
template <class TAtomic, class TDesc, class TItemDescriptionTraits>
class TextAndIconMenuRowTraits
{
public:
  using ValueIconSigPair = QPair<TAtomic, QVariant>;

  static void removeAll(QMenu *w)
  {
    w->clear();

    // Delete the action group not to leave it hanging
    for (auto c : w->children())
      if (auto *ag = qobject_cast<QActionGroup *>(c))
        ag->deleteLater();
  }

  static int getNumberOfRows(QMenu *w)
  {
    return w->actions().count();
  }

  static TAtomic getValueInRow(QMenu *w, int i)
  {
    auto data = w->actions()[i]->data().value<ValueIconSigPair>();
    return data.first;
  }

  static void appendRow(QMenu *w, TAtomic label, const TDesc &desc)
  {
    // The description
    QString text = TItemDescriptionTraits::GetText(label, desc); // QString text(cl.GetLabel());

    // The icon
    QIcon icon = TItemDescriptionTraits::GetIcon(label, desc);

    // The icon signature - a value that can be used to check if the icon has changed
    QVariant iconSig = TItemDescriptionTraits::GetIconSignature(label, desc);

    // Create an action
    QAction *action = new QAction(w);
    action->setText(text);
    action->setIcon(icon);
    action->setData(QVariant::fromValue(ValueIconSigPair(label, iconSig)));
    action->setCheckable(true);

    // Create an action group
    auto *ag = w->actions().size() ? w->actions().first()->actionGroup() : nullptr;
    if(!ag)
      ag = new QActionGroup(w);
    action->setActionGroup(ag);

    // Add the action
    w->addAction(action);
  }

  static void updateRowDescription(QMenu *w, int i, const TDesc &desc)
  {
    // The current value
    QAction *action = w->actions()[i];

    // Get the properies and compare them to the color label
    auto data = action->data().value<ValueIconSigPair>();
    auto label = data.first;

    QVariant newIconSig = TItemDescriptionTraits::GetIconSignature(label, desc);
    if(data.second != newIconSig)
    {
      action->setIcon(TItemDescriptionTraits::GetIcon(label, desc));
      action->setData(QVariant::fromValue(ValueIconSigPair(label, newIconSig)));
    }

    QString newText = TItemDescriptionTraits::GetText(label, desc);
    if(action->text() != newText)
    {
      action->setText(newText);
    }
  }
};

/**
  Use template specialization to generate default traits based on the model
  */
template <class TAtomic, class TDesc>
class DefaultQMenuRowTraits
{
};

template<class TAtomic>
class DefaultQMenuRowTraits<TAtomic, std::string>
  : public TextAndIconMenuRowTraits<TAtomic, std::string, StringRowDescriptionTraits<TAtomic> >
{
};

// Define the defaults
template <class TDomain>
class DefaultWidgetDomainTraits<TDomain, QMenu>
  : public ItemSetWidgetDomainTraits<TDomain, QMenu, DefaultQMenuRowTraits<typename TDomain::ValueType, typename TDomain::DescriptorType>>
{
};

#endif // QTMENUCOUPLING_H
