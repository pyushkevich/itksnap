#ifndef SNAPTESTQT_H
#define SNAPTESTQT_H

#include <SNAPCommon.h>
#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QModelIndex>

class MainImageWindow;
class GlobalUIModel;
class QJSEngine;
class QQmlEngine;
class QTimer;

#if QT_VERSION >= 0x050000
  class QJSEngine;
#else
  class QScriptEngine;
  #define QJSEngine QScriptEngine
#endif



class TestWorker : public QThread
{
  Q_OBJECT

public:
  TestWorker(QObject *parent, QString script, QJSEngine *engine, double accel_factor);

  void run();
  static void sleep_ms(unsigned int msec);

public slots:

  void wait(unsigned int msec);
  void source(QString script_url);

protected:
  QString m_MainScript;
  QJSEngine *m_Engine;

  // Acceleration factor
  double m_Acceleration;

  void readScript(QString script_url, QString &script);

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

  // Invoke a slot
  void invoke(QObject *object, QString slot);

  // Trigger an action_name(by default, in the main menu)
  void trigger(QString action_name, QObject *parent = nullptr);

  // Select an item in a combo box
  void comboBoxSelect(QObject *widget, QString itemText);

  // Thread-safe property/method access (marshaled onto the GUI thread).
  // callMethod/callChildMethod are non-blocking by default (a method could
  // open a nested modal dialog); pass block=true when you know it won't.
  QVariant getProperty(QObject *obj, QString name);
  void setProperty(QObject *obj, QString name, QVariant value);
  void callMethod(QObject *obj, QString method, QVariantList args = QVariantList(), bool block = false);

  // Same, but for a child found by name (avoids a throwaway findChild var)
  QVariant getChildProperty(QObject *parent, QString childName, QString propName);
  void setChildProperty(QObject *parent, QString childName, QString propName, QVariant value);
  void callChildMethod(QObject *parent, QString childName, QString method,
                        QVariantList args = QVariantList(), bool block = false);

  // click/toggle are just callMethod for the two most commonly called slots
  void click(QObject *obj, bool block = false);
  void clickChild(QObject *parent, QString childName, bool block = false);
  void toggle(QObject *obj, bool block = false);
  void close(QObject *obj, bool block = false);
  void closeChild(QObject *parent, QString childName, bool block = false);

  // validateProperty/validateChildProperty fold a getProperty into validateValue
  void validateProperty(QObject *obj, QString name, QVariant expected, double precision = -1.0);
  void validateChildProperty(QObject *parent, QString childName, QString propName,
                              QVariant expected, double precision = -1.0);

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

  // precision < 0 means exact comparison; precision >= 0 means numeric comparison within tolerance
  void validateValue(QVariant v1, QVariant v2, double precision = -1.0);

  void postMouseEvent(QObject *widget, double rel_x, double rel_y, QString eventType, QString button);

  void postKeyEvent(QObject *object, QString key);

  void sleep(int milli_sec);

  static void application_exit(int rc);

protected slots:

  void postKeyEventInternal(QObject *object, QString key);

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

  // Test worker
  TestWorker *m_Worker;

  // Main window pointer
  MainImageWindow *m_Parent;

  // Helper functions
  QModelIndex findItem(QObject *container, QVariant text);
  void printChildrenRecursive(QObject *parent, QString offset, const char *className=NULL);
};

#endif // SNAPTESTQT_H
