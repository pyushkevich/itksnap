#ifndef QTACTIONCOUPLING_H
#define QTACTIONCOUPLING_H

#include <QAction>

/**
  Default traits for a checkable QAction to a boolean
  */
template <>
class DefaultWidgetValueTraits<bool, QAction>
    : public WidgetValueTraitsBase<bool, QAction *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(triggered(bool));
  }

  bool GetValue(QAction *w)
  {
    return w->isChecked();
  }

  void SetValue(QAction *w, const bool &value)
  {
    w->setChecked(value);
  }

  void SetValueToNull(QAction *w)
  {
    w->setChecked(false);
  }

};

#endif // QTACTIONCOUPLING_H
