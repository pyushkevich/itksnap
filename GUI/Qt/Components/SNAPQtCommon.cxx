#include "SNAPQtCommon.h"
#include <QPainter>
#include <QPixmap>
#include <QWidget>
#include <QAction>

QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb)
{
 QRect r(2,2,w-5,w-5);
 QPixmap pix(w, h);
 pix.fill(QColor(0,0,0,0));
 QPainter paint(&pix);
 paint.setPen(Qt::black);
 paint.fillRect(r,rgb);
 paint.drawRect(r);
 return QIcon(pix);
}

QIcon CreateColorBoxIcon(int w, int h, const Vector3ui &rgb)
{
 return CreateColorBoxIcon(w, h, QColor(rgb(0), rgb(1), rgb(2)));
}

QIcon CreateInvisibleIcon(int w, int h)
{
  // Add initial entries to background
  QPixmap pix(w, h);
  pix.fill(QColor(0,0,0,0));
  return QIcon(pix);
}

bool TriggerUpstreamAction(QWidget *widget, const QString &targetActionName)
{
  // Find and execute the relevant action
  QAction *action = widget->window()->findChild<QAction *>("actionLabel_Editor");
  if(action)
    {
    action->trigger();
    return true;
    }
  else
    return false;
}
