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
    qsl.push_back(QString::fromStdString(*it));
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
