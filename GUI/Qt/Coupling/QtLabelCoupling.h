#ifndef QTLABELCOUPLING_H
#define QTLABELCOUPLING_H

#include "QtWidgetCoupling.h"
#include "SNAPQtCommon.h"
#include <iostream>
#include <iomanip>
#include <QLabel>

template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QLabel>
    : public WidgetValueTraitsBase<TAtomic, QLabel *>
{
public:

  virtual TAtomic GetValue(QLabel *w)
  {
    std::istringstream iss(to_utf8(w->text()));
    TAtomic value;
    iss >> value;
    return value;
  }

  virtual void SetValue(QLabel *w, const TAtomic &value)
  {
    std::ostringstream oss;
    oss << value;
    w->setText(from_utf8(oss.str()));
  }

  virtual void SetValueToNull(QLabel *w)
  {
    w->setText("");
  }

  virtual const char *GetSignal()
  {
    return NULL;
  }
};


#endif // QTLABELCOUPLING_H
