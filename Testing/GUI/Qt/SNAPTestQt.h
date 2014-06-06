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
public:
  TestWorker(QObject *parent, QStringList script, QJSEngine *engine);

  void run();

protected:
  QStringList m_Script;
  QJSEngine *m_Engine;
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


  SNAPTestQt(MainImageWindow *win, std::string datadir);
  ~SNAPTestQt();

  ReturnCode RunTest(std::string test);

public slots:

  QObject *findChild(QObject *parent, QString child);

  // Find the contents of an item in a table
  QVariant tableItemText(QObject *table, int row, int col);

  void print(QString text);

  void printChildren(QObject *parent);

  void validateValue(QVariant v1, QVariant v2);

protected:

  ReturnCode ListTests();

  // The data directory for testing
  std::string m_DataDir;

  // We own a script engine
  QJSEngine *m_ScriptEngine;

  // A dummy parent object for this object
  QObject *m_DummyParent;

  // A timer that will execute blocks of code
  QTimer m_Timer;

  // The contents of the script
  QStringList m_ScriptBlocks;
  void printChildrenRecursive(QObject *parent, QString offset);
};

#endif // SNAPTESTQT_H
