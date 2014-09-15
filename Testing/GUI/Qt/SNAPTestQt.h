#ifndef SNAPTESTQT_H
#define SNAPTESTQT_H

#include <SNAPCommon.h>
#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QThread>
#include <QVariant>

class MainImageWindow;
class GlobalUIModel;
class QJSEngine;
class QQmlEngine;
class QTimer;
class QStringList;


class TestWorker : public QThread
{
  Q_OBJECT

public:
  TestWorker(QObject *parent, QString script, QJSEngine *engine, double accel_factor);

  void run();

public slots:

  void wait(unsigned int msec);
  void source(QString script_url);

protected:
  QString m_MainScript;
  QJSEngine *m_Engine;

  // Acceleration factor
  double m_Acceleration;

  void runScript(QString script_url);
  void executeScriptlet(QString scriptlet);
  void processDirective(QString line);
};

class SNAPTestQt : public QObject
{
  Q_OBJECT

public:

  enum ReturnCode {
    SUCCESS = 0,
    EXCEPTION_CAUGHT,
    REGRESSION_TEST_FAILURE,
    NO_SUCH_TEST,
    UNKNOWN_ERROR
    };


  SNAPTestQt(MainImageWindow *win, std::string datadir, double accel_factor);
  ~SNAPTestQt();

  void LaunchTest(std::string test);

public slots:

  // Find a child of an object visible to the script
  QObject *findChild(QObject *parent, QString child);

  // Find a widget by name globally
  QWidget *findWidget(QString widgetName);

  // Return the contents of an item in a table
  QVariant tableItemText(QObject *table, int row, int col);

  // Find the index of an item in a widget (combo, list)
  QVariant findItemRow(QObject *container, QVariant text);

  // Find the index of an item in a widget (combo, list)
  QVariant findItemColumn(QObject *container, QVariant text);

  void print(QString text);

  void printChildren(QObject *parent);

  void printChildren(QObject *parent, QString className);

  void testFailed(QString reason);

  void validateValue(QVariant v1, QVariant v2);

  void validateFloatValue(double v1, double v2, double precision);

  void postMouseEvent(QObject *widget, double rel_x, double rel_y, QString eventType, QString button);

  void postKeyEvent(QObject *object, QString key);
protected:

  ReturnCode ListTests();

  // The data directory for testing
  std::string m_DataDir;

  // We own a script engine
  QJSEngine *m_ScriptEngine;

  // A dummy parent object for this object
  QObject *m_DummyParent;

  // Acceleration factor
  double m_Acceleration;

  // Helper functions
  QModelIndex findItem(QObject *container, QVariant text);
  void printChildrenRecursive(QObject *parent, QString offset, const char *className=NULL);
};

#endif // SNAPTESTQT_H
