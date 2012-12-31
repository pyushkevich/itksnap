#ifndef SNAPQTCOMMON_H
#define SNAPQTCOMMON_H

#include <QIcon>
#include <QObject>
#include <SNAPCommon.h>

class QWidget;
class QAction;

// Generate an icon with a black border and a given fill color
QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb);
QIcon CreateColorBoxIcon(int w, int h, const Vector3ui &rgb);
QIcon CreateInvisibleIcon(int w, int h);

// Find an upstream action for a widget
QAction* FindUpstreamAction(QWidget *w, const QString &targetActionName);

// Connect a widget to the trigger slot in an upstream action
void ConnectWidgetToTopLevelAction(QWidget *w, const char *signal, QString actionName);

// Trigger an upstream action in a Qt widget. Return code is true
// if the upstream action was found, false otherwise
bool TriggerUpstreamAction(QWidget *w, const QString &targetActionName);


// Find a parent window of appropriate class
template <class TWidget>
TWidget *findParentWidget(QObject *w)
{
  do
    {
    w = w->parent();
    if(w)
      {
      TWidget *tw = dynamic_cast<TWidget *>(w);
      if(tw)
        return tw;
      }
    }
  while(w);
  return NULL;
}

#endif // SNAPQTCOMMON_H
