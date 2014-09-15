#include "SNAPTestQt.h"
#include "MainImageWindow.h"

#include <QAction>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QQmlEngine>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QThread>
#include <QRegExp>
#include <QApplication>
#include "SNAPQtCommon.h"

using namespace std;

SNAPTestQt::SNAPTestQt(MainImageWindow *win,
    std::string datadir, double accel_factor)
  : m_Acceleration(accel_factor)
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
    ::exit(SUCCESS);
    }

  // Create and run the thread
  TestWorker *worker = new TestWorker(this, from_utf8(test), m_ScriptEngine, m_Acceleration);
  connect(worker, &TestWorker::finished, worker, &QObject::deleteLater);
  worker->start();
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

#include <QAbstractItemView>

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

#include <QComboBox>

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
    qWarning() << QString("Validation %1 == %2 failed!").arg(v1.toString(),v2.toString());

    // Exit with code corresponding to test failure
    ::exit(REGRESSION_TEST_FAILURE);
    }
  else
    {
    // Validation failed!
    qDebug() << QString("Validation %1 == %2 ok!").arg(v1.toString(),v2.toString());
    }

}

void SNAPTestQt::validateFloatValue(double v1, double v2, double precision)
{
  // Validation involves checking if the values are equal. If not,
  // the program should be halted
  if(fabs(v1 - v2) > precision)
    {
    // Validation failed!
    qWarning() << QString("Validation %1 == %2 (with precision %3) failed!").arg(v1).arg(v2).arg(precision);

    // Exit with code corresponding to test failure
    ::exit(REGRESSION_TEST_FAILURE);
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
  ::exit(REGRESSION_TEST_FAILURE);
}

#include <QMouseEvent>
#include <QApplication>

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
    QPoint point((int)(size.width() * rel_x), (int)(size.height() * rel_y));

    Qt::MouseButton btn = Qt::NoButton;
    if(button == "left")
      btn = Qt::LeftButton;
    else if(button == "right")
      btn = Qt::RightButton;
    else if(button == "middle")
      btn = Qt::MidButton;

    QEvent::Type type = QEvent::None;
    if(eventType == "press")
      type = QEvent::MouseButtonPress;
    else if(eventType == "release")
      type = QEvent::MouseButtonRelease;

    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, point, btn, btn, Qt::NoModifier);
    QApplication::postEvent(widget, event);
    }
}

#include <QKeySequence>
void SNAPTestQt::postKeyEvent(QObject *object, QString key)
{
  QWidget *widget = dynamic_cast<QWidget *>(object);
  if(widget)
    {
    QKeySequence seq(key);
    if(seq.count() == 1)
      {
      int code = seq[0];
      int key = code & 0x01ffffff;
      int mod = code & 0xfe000000;
      Qt::KeyboardModifiers mods;

      if(mod & Qt::ShiftModifier)
        mods |= Qt::ShiftModifier;
      if(mod & Qt::ControlModifier)
        mods |= Qt::ControlModifier;

      QKeyEvent *ev = new QKeyEvent(QEvent::KeyPress, key, mods);
      QApplication::postEvent(widget, ev);
      }
    }
}

SNAPTestQt::ReturnCode
SNAPTestQt::ListTests()
{
  cout << "Available Tests" << endl;
  cout << "  SnakeThreshQt    : Test segmentation with thresholding option" << endl;
  return SUCCESS;
}


TestWorker::TestWorker(QObject *parent, QString script, QJSEngine *engine, double accel_factor)
  : QThread(parent)
{
  m_MainScript = script;
  m_Engine = engine;
  m_Acceleration = accel_factor > 0.0 ? accel_factor : 1.0;
}



void TestWorker::processDirective(QString line)
{
  // If the line is a command, process the command
  QRegExp rxSleep("//---\\s+sleep\\s+(\\d+)");
  QRegExp rxSource("//---\\s+source\\s+(\\w+)");

  if(rxSleep.indexIn(line) >= 0)
    {
    // Sleeping
    int ms = rxSleep.cap(1).toInt() * m_Acceleration;
    qDebug() << QString("Sleeping for %1 ms").arg(ms);
    msleep(ms);
    }

  else if(rxSource.indexIn(line) >= 0)
    {
    // Sleeping
    QString file = rxSource.cap(1);
    this->runScript(file);
    }

  else
    {
    // Unknown directive
    qDebug() << "Unknown directive" << line;
    }
}

void TestWorker::executeScriptlet(QString scriptlet)
{
  QJSValue rc = m_Engine->evaluate(scriptlet);
  if(rc.isError())
    {
    qWarning() << "JavaScript exception:" << rc.toString();
    ::exit(SNAPTestQt::REGRESSION_TEST_FAILURE);
    }
}


void TestWorker::runScript(QString script_url)
{
  // The test may be a path to an actual file
  if(!QFileInfo(script_url).isReadable())
    script_url = QString(":/scripts/Scripts/test_%1.js").arg(script_url);

  // Report which test we are accessing
  qDebug() << "Running test " << script_url;

  // Find the script file corresponding to the test
  QFile file(script_url);
  if(!file.open(QIODevice::ReadOnly))
    {
    qWarning() << QString("Unable to read test script %1").arg(script_url);
    ::exit(SNAPTestQt::NO_SUCH_TEST);
    }

  // Read the script
  QTextStream stream(&file);

  // Break the script into pieces that should be sent to the processor
  QString scriptlet;
  while(true)
    {
    // Read a line of the script
    QString line = stream.readLine();

    // Is the line an interpreter command or a script line?
    if(line.isNull() || line.startsWith("//---"))
      {
      // The current scriptlet has ended. Time to execute!
      this->executeScriptlet(scriptlet);

      // Reset the scriptlet
      scriptlet = QString();

      // If the line is null (eof) break
      if(line.isNull())
        break;

      // Otherwise, it's a directive
      this->processDirective(line);
      }
    else
      {
      // Append the line to the current scriptlent
      scriptlet.append(line);
      scriptlet.append('\n');
      }
    }

  // That's it - the script is finished
}

void TestWorker::run()
{
  // Add ourselves to the engine
  QJSValue mwin = m_Engine->newQObject(this);
  m_Engine->globalObject().setProperty("thread", mwin);

  // Make sure full output is captured
  qDebug() << "CTEST_FULL_OUTPUT";

  // Run the top-level script
  // runScript(m_MainScript);
  source(m_MainScript);

  // Once the test has completed, we can exit the application
  ::exit(SNAPTestQt::SUCCESS);
}

void TestWorker::wait(unsigned int msec)
{
  msleep(msec);
}

void TestWorker::source(QString script_url)
{
  // The test may be a path to an actual file
  if(!QFileInfo(script_url).isReadable())
    script_url = QString(":/scripts/Scripts/test_%1.js").arg(script_url);

  // Report which test we are accessing
  qDebug() << "Running test " << script_url;

  // Find the script file corresponding to the test
  QFile file(script_url);
  if(!file.open(QIODevice::ReadOnly))
    {
    qWarning() << QString("Unable to read test script %1").arg(script_url);
    ::exit(SNAPTestQt::NO_SUCH_TEST);
    }

  // Read the script
  QTextStream stream(&file);
  QString script;

  // Read the script line by line, making substitutions
  while(!stream.atEnd())
    {
    QString line = stream.readLine();
    QRegExp rxSleep("^\\s*$");
    QRegExp rxComment("//===\\s+(\\w+.*)");

    if(rxSleep.indexIn(line) >= 0)
      {
      line = QString("thread.wait(500)");
      }
    else if(rxComment.indexIn(line) >= 0)
      {
      line = QString("engine.print(\"%1\")").arg(rxComment.cap(1));
      }

    script += line;
    script += "\n";
    }

  // Close the file
  file.close();

  // Execute it
  QJSValue rc = m_Engine->evaluate(script);
  if(rc.isError())
    {
    qWarning() << "JavaScript exception:" << rc.toString();
    ::exit(SNAPTestQt::REGRESSION_TEST_FAILURE);
    }
}
