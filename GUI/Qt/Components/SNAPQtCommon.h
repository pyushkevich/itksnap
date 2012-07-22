#ifndef SNAPQTCOMMON_H
#define SNAPQTCOMMON_H

#include <QIcon>
#include <SNAPCommon.h>

class QWidget;

// Generate an icon with a black border and a given fill color
QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb);
QIcon CreateColorBoxIcon(int w, int h, const Vector3ui &rgb);
QIcon CreateInvisibleIcon(int w, int h);

// Trigger an upstream action in a Qt widget. Return code is true
// if the upstream action was found, false otherwise
bool TriggerUpstreamAction(QWidget *w, const QString &targetActionName);


#endif // SNAPQTCOMMON_H
