#ifndef SNAPQTCOMMON_H
#define SNAPQTCOMMON_H

#include <QBrush>
#include <QIcon>
#include <QObject>
#include <SNAPCommon.h>

class QWidget;
class QAction;
class GlobalUIModel;
class QMenu;
class QSlider;
class QSpinBox;
class ColorLabelTable;
class ColorLabel;
class ColorMap;

// Generate an icon with a black border and a given fill color
QIcon CreateColorBoxIcon(int w, int h, const QBrush &brush);
QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb);
QIcon CreateColorBoxIcon(int w, int h, const Vector3ui &rgb);
QIcon CreateInvisibleIcon(int w, int h);

// This creates an icon for a given color map. This function uses internal
// caching so that the icon is only regenerated if the color map has been
// modified since the last time the icon was generated.
QIcon CreateColorMapIcon(int w, int h, ColorMap *cmap);

// Generate a brush corresponding to a color label
QBrush GetBrushForColorLabel(const ColorLabel &cl);
QBrush GetBrushForDrawOverFilter(DrawOverFilter flt, const ColorLabel &cl);
QString GetTitleForColorLabel(const ColorLabel &cl);
QString GetTitleForDrawOverFilter(DrawOverFilter flt, const ColorLabel &cl);

QBrush GetBrushForColorLabel(int label, ColorLabelTable *clt);
QBrush GetBrushForDrawOverFilter(DrawOverFilter flt, ColorLabelTable *clt);
QString GetTitleForColorLabel(int label, ColorLabelTable *clt);
QString GetTitleForDrawOverFilter(DrawOverFilter flt, ColorLabelTable *clt);

// Find an upstream action for a widget
QAction* FindUpstreamAction(QWidget *w, const QString &targetActionName);

// Connect a widget to the trigger slot in an upstream action
void ConnectWidgetToTopLevelAction(QWidget *w, const char *signal, QString actionName);

// Trigger an upstream action in a Qt widget. Return code is true
// if the upstream action was found, false otherwise
bool TriggerUpstreamAction(QWidget *w, const QString &targetActionName);

// Convert a container of std::strings into a QStringList
QStringList toQStringList(const std::vector<std::string> inlist);

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

// Couple a slider and a

// Standard way for handling non-lethal exceptions
void ReportNonLethalException(QWidget *parent,
                              std::exception &exc,
                              QString windowTitleText,
                              QString mainErrorText);

/** Populate a menu with history items */
void PopulateHistoryMenu(
    QMenu *menu, QObject *receiver, const char *slot,
    GlobalUIModel *model, QString hist_category);

void PopulateHistoryMenu(
    QMenu *menu, QObject *receiver, const char *slot,
    const QStringList &local_history, const QStringList &global_history);


/** Show a generic file save dialog with a history dropdown */
QString ShowSimpleSaveDialogWithHistory(
    GlobalUIModel *model, QString hist_category,
    QString window_title, QString file_title, QString file_pattern);

/** Show a generic file open dialog with a history dropdown */
QString ShowSimpleOpenDialogWithHistory(
    GlobalUIModel *model, QString hist_category,
    QString window_title, QString file_title, QString file_pattern);

/** Convert a QString to a std::string using UTF8 encoding */
inline std::string to_utf8(const QString &qstring)
{
  return std::string(qstring.toUtf8().constData());
}

/** Convert an std::string with UTF8 encoding to a Qt string */
inline QString from_utf8(const std::string &input)
{
  return QString::fromUtf8(input.c_str());
}

#endif // SNAPQTCOMMON_H
