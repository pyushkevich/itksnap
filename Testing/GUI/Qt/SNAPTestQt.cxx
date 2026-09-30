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
#include <QMetaMethod>


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
  setProperty(widget, "currentIndex", row);
}

// Property get/set block until the GUI thread has run them, so the script
// sees the result (and anything it triggers) immediately after
QVariant SNAPTestQt::getProperty(QObject *obj, QString name)
{
  QVariant result;
  QByteArray prop = name.toUtf8();
  QMetaObject::invokeMethod(obj, [obj, prop, &result]() {
      result = obj->property(prop.constData());
    }, Qt::BlockingQueuedConnection);
  return result;
}

void SNAPTestQt::setProperty(QObject *obj, QString name, QVariant value)
{
  QByteArray prop = name.toUtf8();
  QMetaObject::invokeMethod(obj, [obj, prop, value]() {
      obj->setProperty(prop.constData(), value);
    }, Qt::BlockingQueuedConnection);
}

// Default is non-blocking (a method call could open a nested modal dialog,
// which would deadlock a blocking connection) -- pass block=true only for
// calls you know don't do that, to replace a guessed sleep() with a real
// synchronization point. Resolved via QMetaMethod so any argument type/count
// the target actually declares works, not just a hardcoded set.
void SNAPTestQt::callMethod(QObject *obj, QString method, QVariantList args, bool block)
{
  Qt::ConnectionType ct = block ? Qt::BlockingQueuedConnection : Qt::QueuedConnection;
  const QMetaObject *mo = obj->metaObject();
  QByteArray name = method.toUtf8();

  for(int i = 0; i < mo->methodCount(); i++)
    {
    QMetaMethod mm = mo->method(i);
    if(mm.name() != name || mm.parameterCount() != args.size())
      continue;

    QVariant converted[5];
    QGenericArgument ga[5];
    for(int j = 0; j < args.size(); j++)
      {
      converted[j] = args[j];
      converted[j].convert(QMetaType(mm.parameterType(j)));
      ga[j] = QGenericArgument(mm.parameterTypeName(j), converted[j].data());
      }

    mm.invoke(obj, ct, ga[0], ga[1], ga[2], ga[3], ga[4]);
    return;
    }

  m_ScriptEngine->throwError(QJSValue::ReferenceError,
                             QString("No method %1/%2 found").arg(method).arg(args.size()));
}

QVariant SNAPTestQt::getChildProperty(QObject *parent, QString childName, QString propName)
{
  return getProperty(findChild(parent, childName), propName);
}

void SNAPTestQt::setChildProperty(QObject *parent, QString childName, QString propName, QVariant value)
{
  setProperty(findChild(parent, childName), propName, value);
}

void SNAPTestQt::callChildMethod(QObject *parent, QString childName, QString method, QVariantList args, bool block)
{
  callMethod(findChild(parent, childName), method, args, block);
}

void SNAPTestQt::click(QObject *obj, bool block)
{
  callMethod(obj, "click", QVariantList(), block);
}

void SNAPTestQt::clickChild(QObject *parent, QString childName, bool block)
{
  click(findChild(parent, childName), block);
}

void SNAPTestQt::toggle(QObject *obj, bool block)
{
  callMethod(obj, "toggle", QVariantList(), block);
}

void SNAPTestQt::close(QObject *obj, bool block)
{
  callMethod(obj, "close", QVariantList(), block);
}

void SNAPTestQt::closeChild(QObject *parent, QString childName, bool block)
{
  close(findChild(parent, childName), block);
}

void SNAPTestQt::validateProperty(QObject *obj, QString name, QVariant expected, double precision)
{
  validateValue(getProperty(obj, name), expected, precision);
}

void SNAPTestQt::validateChildProperty(QObject *parent, QString childName, QString propName,
                                        QVariant expected, double precision)
{
  validateValue(getChildProperty(parent, childName, propName), expected, precision);
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

void SNAPTestQt::validateValue(QVariant v1, QVariant v2, double precision)
{
  // precision < 0 means exact comparison; otherwise numeric within tolerance
  bool failed;
  QString msg;
  if(precision >= 0)
    {
    failed = fabs(v1.toDouble() - v2.toDouble()) > precision;
    msg = QString("Validation %1 == %2 (with precision %3) %4!")
        .arg(v1.toDouble()).arg(v2.toDouble()).arg(precision).arg(failed ? "failed" : "ok");
    }
  else
    {
    failed = v1 != v2;
    msg = QString("Validation %1 == %2 %3!").arg(v1.toString(), v2.toString(), failed ? "failed" : "ok");
    }

  if(failed)
    {
    qWarning() << msg;
    m_ScriptEngine->throwError(QJSValue::GenericError, msg);
    }
  else
    {
    qDebug() << msg;
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
