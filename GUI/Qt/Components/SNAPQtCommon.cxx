#include "SNAPQtCommon.h"
#include <QPainter>
#include <QPixmap>
#include <QWidget>
#include <QAction>
#include <QObject>
#include <QPushButton>
#include <QMainWindow>
#include <QMessageBox>
#include <QApplication>
#include <QMenu>

#include "GlobalUIModel.h"
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "HistoryManager.h"
#include "SimpleFileDialogWithHistory.h"
#include "ColorLabelTable.h"

QIcon CreateColorBoxIcon(int w, int h, const QBrush &brush)
{
  QRect r(2,2,w-5,w-5);
  QPixmap pix(w, h);
  pix.fill(QColor(0,0,0,0));
  QPainter paint(&pix);
  paint.setPen(Qt::black);
  paint.setBrush(brush);
  paint.drawRect(r);
  return QIcon(pix);
}

QIcon CreateColorBoxIcon(int w, int h, const QColor &rgb)
{
  return CreateColorBoxIcon(w, h, QBrush(rgb));
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

QBrush GetBrushForColorLabel(const ColorLabel &cl)
{
  return QBrush(QColor(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
}

QBrush GetBrushForDrawOverFilter(DrawOverFilter flt, const ColorLabel &cl)
{
  switch(flt.CoverageMode)
    {
    case PAINT_OVER_VISIBLE:
      return QBrush(Qt::black, Qt::Dense6Pattern);
    case PAINT_OVER_ONE:
      return QBrush(QColor(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
    case PAINT_OVER_ALL:
      return QBrush(Qt::black, Qt::BDiagPattern);
    }
}

QString GetTitleForColorLabel(const ColorLabel &cl)
{
  return QString::fromUtf8(cl.GetLabel());
}

QString GetTitleForDrawOverFilter(DrawOverFilter flt, const ColorLabel &cl)
{
  switch(flt.CoverageMode)
    {
    case PAINT_OVER_VISIBLE:
      return QString("All visible labels");
    case PAINT_OVER_ONE:
      return QString::fromUtf8(cl.GetLabel());
    case PAINT_OVER_ALL:
      return QString("All labels");
    }
}

QBrush GetBrushForColorLabel(int label, ColorLabelTable *clt)
{
  return GetBrushForColorLabel(clt->GetColorLabel(label));
}

QBrush GetBrushForDrawOverFilter(DrawOverFilter flt, ColorLabelTable *clt)
{
  return GetBrushForDrawOverFilter(flt, clt->GetColorLabel(flt.DrawOverLabel));
}

QString GetTitleForColorLabel(int label, ColorLabelTable *clt)
{
  return GetTitleForColorLabel(clt->GetColorLabel(label));
}

QString GetTitleForDrawOverFilter(DrawOverFilter flt, ColorLabelTable *clt)
{
  return GetTitleForDrawOverFilter(flt, clt->GetColorLabel(flt.DrawOverLabel));
}







QAction *FindUpstreamAction(QWidget *widget, const QString &targetActionName)
{
  // Look for a parent of QMainWindow type
  QMainWindow *topwin = NULL;
  for(QObject *p = widget; p != NULL; p = p->parent())
    {
    if((topwin = dynamic_cast<QMainWindow *>(p)) != NULL)
      break;
    }

  // If nothing found, try a global search
  if(!topwin)
    {
    QWidgetList lst = QApplication::topLevelWidgets();
    for(QWidgetList::Iterator it = lst.begin();
        it != lst.end(); ++it)
      {
      QWidget *w = *it;
      if((topwin = dynamic_cast<QMainWindow *>(w)) != NULL)
        break;
      }
    }

  // Look for the action
  QAction *result = NULL;
  if(topwin)
    {
    result = topwin->findChild<QAction *>(targetActionName);
    }

  if(!result)
      std::cerr << "Failed find upstream action " << targetActionName.toStdString() << std::endl;

  return result;
}

void ConnectWidgetToTopLevelAction(
    QWidget *w, const char *signal, QString actionName)
{
  QAction *action = FindUpstreamAction(w, actionName);
  QObject::connect(w, signal, action, SLOT(trigger()));
}

bool TriggerUpstreamAction(QWidget *widget, const QString &targetActionName)
{
  // Find and execute the relevant action
  QAction *action = FindUpstreamAction(widget, targetActionName);
  if(action)
    {
    action->trigger();
    return true;
    }
  else
    return false;
}

QStringList toQStringList(const std::vector<std::string> inlist)
{
  QStringList qsl;
  qsl.reserve(inlist.size());
  for(std::vector<std::string>::const_iterator it = inlist.begin();
      it != inlist.end(); ++it)
    {
    qsl.push_back(from_utf8(*it));
    }
  return qsl;
}

void ReportNonLethalException(QWidget *parent,
                              std::exception &exc,
                              QString windowTitleText,
                              QString mainErrorText)
{
  QMessageBox b(parent);
  b.setText(mainErrorText);
  b.setWindowTitle(QString("%1 - ITK-SNAP").arg(windowTitleText));
  b.setDetailedText(exc.what());
  b.setIcon(QMessageBox::Critical);
  b.exec();
}

void PopulateHistoryMenu(
    QMenu *menu, QObject *receiver, const char *slot,
    const QStringList &local_history,
    const QStringList &global_history)
{
  menu->clear();

  QStringListIterator itLocal(local_history);
  itLocal.toBack();
  while(itLocal.hasPrevious())
    menu->addAction(itLocal.previous(), receiver, slot);

  int nLocal = menu->actions().size();

  QStringListIterator itGlobal(global_history);
  itGlobal.toBack();
  while(itGlobal.hasPrevious())
    {
    QString entry = itGlobal.previous();
    if(local_history.indexOf(entry) == -1)
      menu->addAction(entry, receiver, slot);
    }

  if(nLocal > 0 && menu->actions().size() > nLocal)
    menu->insertSeparator(menu->actions()[nLocal]);
}


void PopulateHistoryMenu(
    QMenu *menu, QObject *receiver, const char *slot,
    GlobalUIModel *model, QString hist_category)
{
  HistoryManager *hm =
      model->GetDriver()->GetSystemInterface()->GetHistoryManager();

  QStringList hl = toQStringList(hm->GetLocalHistory(hist_category.toStdString()));
  QStringList hg = toQStringList(hm->GetGlobalHistory(hist_category.toStdString()));
  PopulateHistoryMenu(menu, receiver, slot, hl, hg);
}



/** Show a generic file save dialog with a history dropdown */
QString ShowSimpleSaveDialogWithHistory(
    GlobalUIModel *model, QString hist_category,
    QString window_title, QString file_title, QString file_pattern)
{
  HistoryManager *hm =
      model->GetDriver()->GetSystemInterface()->GetHistoryManager();

  QStringList hl = toQStringList(hm->GetLocalHistory(hist_category.toStdString()));
  QStringList hg = toQStringList(hm->GetGlobalHistory(hist_category.toStdString()));

  return SimpleFileDialogWithHistory::showSaveDialog(
        window_title, file_title, hl, hg, file_pattern);
}

/** Show a generic file open dialog with a history dropdown */
QString ShowSimpleOpenDialogWithHistory(
    GlobalUIModel *model, QString hist_category,
    QString window_title, QString file_title, QString file_pattern)
{
  HistoryManager *hm =
      model->GetDriver()->GetSystemInterface()->GetHistoryManager();

  QStringList hl = toQStringList(hm->GetLocalHistory(hist_category.toStdString()));
  QStringList hg = toQStringList(hm->GetGlobalHistory(hist_category.toStdString()));

  return SimpleFileDialogWithHistory::showOpenDialog(
        window_title, file_title, hl, hg, file_pattern);
}


