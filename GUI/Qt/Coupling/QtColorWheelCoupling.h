#ifndef QTCOLORWHEELCOUPLING_H
#define QTCOLORWHEELCOUPLING_H

#include "ColorWheel.h"

std::ostream &operator << (std::ostream &out, QColor color)
{
  out << color.red() << ", " << color.green() << ", " << color.blue();
  return out;
}

template <class TColorRep>
class DefaultWidgetValueTraits< iris_vector_fixed<TColorRep, 3>, ColorWheel>
    : public WidgetValueTraitsBase<iris_vector_fixed<TColorRep, 3>, ColorWheel *>
{
public:

  typedef iris_vector_fixed<TColorRep, 3> ColorVec;

  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(colorChange(const QColor &));
  }

  ColorVec GetValue(ColorWheel *w)
  {
    QColor qclr = w->color();
    return ColorVec(qclr.red(), qclr.green(), qclr.blue());
  }

  void SetValue(ColorWheel *w, const ColorVec &value)
  {
    // We have to actually find the item
    QColor newcol(value[0],value[1],value[2]);
    if(newcol != w->color())
      {
      w->setColor(newcol);
      }
  }

  void SetValueToNull(ColorWheel *w)
  {
    w->setColor(QColor());
  }
};



#endif // QTCOLORWHEELCOUPLING_H
