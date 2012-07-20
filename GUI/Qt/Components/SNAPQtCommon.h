#ifndef SNAPQTCOMMON_H
#define SNAPQTCOMMON_H

#include <QIcon>
#include <SNAPCommon.h>

// Generate an icon with a black border and a given fill color
QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb);
QIcon CreateColorBoxIcon(int w, int h, const Vector3ui &rgb);

#endif // SNAPQTCOMMON_H
