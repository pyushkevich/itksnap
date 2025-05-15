#include "SNAPTestQt.h"
#include "MainImageWindow.h"

#include <QAction>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QMouseEvent>
#include <QApplication>
#include <QKeySequence>
#include <QDir>
#include <SNAPQApplication.h>
#include <QDeadlineTimer>


#include "SNAPQtCommon.h"

#if QT_VERSION >= 0x050000
  #include <QJSEngine>
#else
  #include <QScriptEngine>
  #define QJSEngine QScriptEngine
  #define QJSValue QScriptValue
#endif

using namespace std;

SNAPTestQt::SNAPTestQt(MainImageWindow *win,
    std::string datadir, double accel_factor)
: m_Acceleration(accel_factor), m_Parent(win)
{
  // We need a dummy parent to prevent self-deletion
  m_DummyParent = new QObject();
  this->setParent(m_DummyParent);

  // Create the script engine
  m_ScriptEngine = new QJSEngine();

  // Assign the window as a variable in the script engine
  QJSValue mwin = m_ScriptEngine->newQObject(win);
  m_ScriptEngine->globalObject().setProperty("mainwin", mwin);

  // Provide a pointer to the engine
  QJSValue vthis = m_ScriptEngine->newQObject(this);
  m_ScriptEngine->globalObject().setProperty("engine", vthis);

  QJSValue test = m_ScriptEngine->newQObject(win->findChild<QPushButton *>("btnLoadMain"));
  m_ScriptEngine->globalObject().setProperty("btn", test);

  // Assign the data directory to the script engine
  m_ScriptEngine->globalObject().setProperty("datadir", from_utf8(datadir));
}

SNAPTestQt::~SNAPTestQt()
{
  delete m_ScriptEngine;
  setParent(NULL);
  delete m_DummyParent;
}

#include <QFileInfo>

void
SNAPTestQt::LaunchTest(std::string test)
{
  // Special case: listing all tests
  if(test == "list")
    {
    ListTests();
    application_exit(SUCCESS);
    }

  // Create and run the thread
  m_Worker = new TestWorker(this, from_utf8(test), m_ScriptEngine, m_Acceleration);

  connect(m_Worker, SIGNAL(finished()), m_Worker, SLOT(deleteLater()));

  m_Worker->start();
}

QObject *SNAPTestQt::findChild(QObject *parent, QString child)
{
  return parent->findChild<QObject *>(child);
}

QWidget *SNAPTestQt::findWidget(QString widgetName)
{
  foreach(QWidget *w, QApplication::allWidgets())
    if(w->objectName() == widgetName)
      return w;

  return NULL;
}


QVariant SNAPTestQt::tableItemText(QObject *table, int row, int col)
{
  QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(table);
  if(view)
    {
    QAbstractItemModel *model = view->model();
    return model->data(model->index(row, col));
    }

  return QVariant();
}


QModelIndex SNAPTestQt::findItem(QObject *container, QVariant text)
{
  QAbstractItemModel *model = NULL;

  // Is it a combo box?
  if(QComboBox *combo = dynamic_cast<QComboBox *>(container))
    model = combo->model();

  // Is it an item view?
  else if(QAbstractItemView *itemview = dynamic_cast<QAbstractItemView *>(container))
    model = itemview->model();

  // Find the item
  if(model)
    {
    QModelIndexList found = model->match(model->index(0,0),Qt::DisplayRole,text);
    if(found.size())
      return found.at(0);
    }

  return QModelIndex();
}

void SNAPTestQt::invoke(QObject *object, QString slot)
{
  if(!object)
    m_ScriptEngine->throwError(QJSValue::ReferenceError,
                               QString("Invoked slot %1 on null object").arg(slot));
  else
    QMetaObject::invokeMethod(object, slot.toStdString().c_str(), Qt::QueuedConnection);
}

void SNAPTestQt::trigger(QString action_name, QObject *parent)
{
  auto *action = dynamic_cast<QAction *>(findChild(parent ? parent : this->m_Parent, action_name));
  invoke(action, "trigger");
}

void SNAPTestQt::comboBoxSelect(QObject *widget, QString itemText)
{
  auto *combo = dynamic_cast<QComboBox *>(widget);
  if(!combo)
    m_ScriptEngine->throwError(QJSValue::ReferenceError,
                               QString("comboBoxSelect target not a combo box"));

  int row = findItemRow(widget, itemText).toInt();
  QMetaObject::invokeMethod(widget, "setCurrentIndex", Qt::QueuedConnection, Q_ARG(int, row));
}


QVariant SNAPTestQt::findItemRow(QObject *container, QVariant text)
{
  QModelIndex idx = findItem(container, text);
  if(idx.isValid())
    return idx.row();

  return QVariant();
}

QVariant SNAPTestQt::findItemColumn(QObject *container, QVariant text)
{
  QModelIndex idx = findItem(container, text);
  if(idx.isValid())
    return idx.column();

  return QVariant();
}


void SNAPTestQt::print(QString text)
{
  qDebug() << text;
}

void SNAPTestQt::printChildrenRecursive(QObject *parent, QString offset, const char *className)
{
  if(parent)
    {
    if(!className || parent->inherits(className))
      {
      QString line = QString("%1%2 : %3").arg(offset,parent->metaObject()->className(),parent->objectName());
      qDebug() << line;
      }

    foreach (QObject* child, parent->children())
      {
      QWidget *widget = dynamic_cast<QWidget *>(child);
      if(widget)
        printChildrenRecursive(child, offset + "  ", className);
      }
    }
  else
    {
    qDebug() << "NULL passed to printChild";
    }
}

void SNAPTestQt::printChildren(QObject *parent)
{
  printChildrenRecursive(parent, "");
}

void SNAPTestQt::printChildren(QObject *parent, QString className)
{
  const char *cn = NULL;
  if(!className.isNull())
    {
    QByteArray ba = className.toLocal8Bit();
    cn = ba.data();
    }
  printChildrenRecursive(parent, "", cn);
}

void SNAPTestQt::validateValue(QVariant v1, QVariant v2)
{
  // Validation involves checking if the values are equal. If not,
  // the program should be halted
  if(v1 != v2)
    {
    // Validation failed!
    QString msg = QString("Validation %1 == %2 failed!").arg(v1.toString(),v2.toString());
    qWarning() << msg;

    // Exit with code corresponding to test failure
    m_ScriptEngine->throwError(QJSValue::GenericError, msg);
    // application_exit(REGRESSION_TEST_FAILURE);
    }
  else
    {
    // Validation failed!
    qDebug() << QString("Validation %1 == %2 ok!").arg(v1.toString(),v2.toString());
    }

}

void SNAPTestQt::application_exit(int rc)
{
  QMetaObject::invokeMethod(
        QCoreApplication::instance(), "quitWithReturnCode", Qt::QueuedConnection,
        Q_ARG(int, rc));
}

void SNAPTestQt::postKeyEventInternal(QObject *object, QString key)
{
    QWidget *widget = dynamic_cast<QWidget *>(object);
    if(widget)
    {
        QKeySequence seq(key);
        if(seq.count() == 1)
        {
            QKeyCombination code = seq[0];
            Qt::Key key = code.key();
            Qt::KeyboardModifiers mods = code.keyboardModifiers();

            QKeyEvent *ev = new QKeyEvent(QEvent::KeyPress, key, mods);
            QApplication::postEvent(widget, ev);
        }
    }
}

void SNAPTestQt::sleep(int milli_sec)
{
  // Scale requested sleep time by acceleration factor
  int ms_actual = (int)(milli_sec / m_Acceleration);

  // Sleep
  TestWorker::sleep_ms(ms_actual);
}

void SNAPTestQt::validateFloatValue(double v1, double v2, double precision)
{
  // Validation involves checking if the values are equal. If not,
  // the program should be halted
  if(fabs(v1 - v2) > precision)
    {
    // Validation failed!
    QString msg = QString("Validation %1 == %2 (with precision %3) failed!").arg(v1).arg(v2).arg(precision);
    qWarning() << msg;

    // Exit with code corresponding to test failure
    m_ScriptEngine->throwError(QJSValue::GenericError, msg);

    // application_exit(REGRESSION_TEST_FAILURE);
    }
  else
    {
    // Validation failed!
    qDebug() << QString("Validation %1 == %2 (with precision %3) ok!").arg(v1).arg(v2).arg(precision);
    }
}

void SNAPTestQt::testFailed(QString reason)
{
  qWarning() << reason;
  m_ScriptEngine->throwError(QJSValue::GenericError, reason);
  // application_exit(REGRESSION_TEST_FAILURE);
}


void SNAPTestQt::postMouseEvent(QObject *object, double rel_x, double rel_y, QString eventType, QString button)
{
  // Special case handlers
  if(eventType == "click")
    {
    postMouseEvent(object, rel_x, rel_y, "press", button);
    postMouseEvent(object, rel_x, rel_y, "release", button);
    return;
    }

  QWidget *widget = dynamic_cast<QWidget *>(object);
  if(widget)
    {
    QSize size = widget->size();
    QPointF localPos((int)(size.width() * rel_x), (int)(size.height() * rel_y));
    QPointF globalPos = widget->mapToGlobal(localPos); // added global pos to fix deprected QMouseEvent Constructor issue

    Qt::MouseButton btn = Qt::NoButton;
    if(button == "left")
      btn = Qt::LeftButton;
    else if(button == "right")
      btn = Qt::RightButton;
    else if(button == "middle")
      btn = Qt::MiddleButton;

    QEvent::Type type = QEvent::None;
    if(eventType == "press")
      type = QEvent::MouseButtonPress;
    else if(eventType == "release")
      type = QEvent::MouseButtonRelease;

    QMouseEvent *event = new QMouseEvent(type, localPos, globalPos, btn, btn, Qt::NoModifier);
    QApplication::postEvent(widget, event);
    }
}

void SNAPTestQt::postKeyEvent(QObject *object, QString key)
{
  // We need the code to run in the main thread
  QMetaObject::invokeMethod(
    this, "postKeyEventInternal", Qt::QueuedConnection, Q_ARG(QObject *, object), Q_ARG(QString, key));
}


SNAPTestQt::ReturnCode
SNAPTestQt::ListTests()
{
  QDir script_dir(":/scripts/Scripts");
  QStringList filters; filters << "test_*.js";
  script_dir.setNameFilters(filters);
  QStringList files = script_dir.entryList();

  QRegularExpression rx("test_(.*).js");

  cout << "Available Tests" << endl;
  foreach(const QString &test, files)
    {
    auto rm = rx.match(test);
    if(rm.hasMatch())
      cout << "  " << rm.captured(1).toStdString() << endl;
    }

  return SUCCESS;
}


TestWorker::TestWorker(QObject *parent, QString script, QJSEngine *engine, double accel_factor)
  : QThread(parent)
{
  m_MainScript = script;
  m_Engine = engine;
  m_Acceleration = accel_factor > 0.0 ? accel_factor : 1.0;
}

void TestWorker::run()
{
  // Add ourselves to the engine
  QJSValue mwin = m_Engine->newQObject(this);
  m_Engine->globalObject().setProperty("thread", mwin);

  // Make sure full output is captured
  qDebug() << "CTEST_FULL_OUTPUT";

  // Run the top-level script
  source(m_MainScript);
}

void TestWorker::sleep_ms(unsigned int msec)
{
  QThread::msleep(msec);
}

void TestWorker::wait(unsigned int msec)
{
  msleep(msec);
}

void TestWorker::readScript(QString script_url, QString &script)
{
  // Find the script file corresponding to the test
  QFile file(script_url);
  if(!file.open(QIODevice::ReadOnly))
    {
    qWarning() << QString("Unable to read test script %1").arg(script_url);
    SNAPTestQt::application_exit(SNAPTestQt::NO_SUCH_TEST);
    }

  // Read the script
  QTextStream stream(&file);

  // Read the script line by line, making substitutions
  while(!stream.atEnd())
    {
    QString line = stream.readLine();
    auto rmSleep = QRegularExpression("^\\s*$").match(line);
    auto rmComment = QRegularExpression("//===\\s+(\\w+.*)").match(line);
    // QRegExp rxInclude("include.*\\((\\w+.*)\\)");
    auto rmInclude = QRegularExpression("include.*\"(\\w+.*)\".*").match(line);

    if(rmSleep.hasMatch())
      {
      line = QString("engine.sleep(500)");
      }
    else if(rmComment.hasMatch())
      {
      line = QString("engine.print(\"%1\")").arg(rmComment.captured(1));
      }
    else if(rmInclude.hasMatch())
      {
      QString child_url = rmInclude.captured(1);
      if(!QFileInfo(child_url).isReadable())
        child_url = QString(":/scripts/Scripts/test_%1.js").arg(child_url);

      qDebug() << "Including : " << child_url;

      this->readScript(child_url, script);
      line = "";
      }

    script += line;
    script += "\n";
    }

  // Close the file
  file.close();
}

void TestWorker::source(QString script_url)
{
  // The test may be a path to an actual file
  if(!QFileInfo(script_url).isReadable())
    script_url = QString(":/scripts/Scripts/test_%1.js").arg(script_url);

  // Report which test we are accessing
  qDebug() << "Running test: " << script_url;

  QString script;
  this->readScript(script_url, script);

  // Execute it
  QJSValue rc = m_Engine->evaluate(script);
  qWarning() << "Return code from evaluate is " << rc.toString();
  if(rc.isError())
    {
    qWarning() << "JavaScript exception:" << rc.toString();
    SNAPTestQt::application_exit(SNAPTestQt::EXCEPTION_CAUGHT);
    }
  else
    {
    qDebug() << "Successfully completed test script with return code " << rc.toString();
    SNAPTestQt::application_exit(SNAPTestQt::SUCCESS);
    }
}
