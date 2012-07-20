#include "SNAPQtCommon.h"
#include <QPainter>
#include <QPixmap>

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

