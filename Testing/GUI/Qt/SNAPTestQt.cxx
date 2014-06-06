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
#include <QCoreApplication>
#include "SNAPQtCommon.h"

using namespace std;

SNAPTestQt::SNAPTestQt(
    MainImageWindow *win,
    std::string datadir)
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

  // Hook up the timer
  connect(&m_Timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

SNAPTestQt::~SNAPTestQt()
{
  delete m_ScriptEngine;
  setParent(NULL);
  delete m_DummyParent;
}

#include <QFileInfo>

SNAPTestQt::ReturnCode
SNAPTestQt::RunTest(std::string test)
{
  if(test == "list")
    return ListTests();

  // The test may be a path to an actual file
  QString url = from_utf8(test);
  if(!QFileInfo(url).isReadable())
    url = QString(":/scripts/Scripts/test_%1.js").arg(from_utf8(test));

  // Report which test we are accessing
  qDebug() << "Running test " << url;

  // Find the script file corresponding to the test
  QFile file(url);
  if(!file.open(QIODevice::ReadOnly))
    return NO_SUCH_TEST;

  // Read the script
  QTextStream stream(&file);

  // Break the script into pieces
  m_ScriptBlocks.clear();

  while(!stream.atEnd())
    {
    // Read from the stream until we find a block of code
    while(!stream.atEnd())
      {
      QString line = stream.readLine();
      if(line.isNull() || line.startsWith("//<--"))
        break;
      }

    // Read and execute the lines of code inside the block
    QString script;
    while(!stream.atEnd())
      {
      QString line = stream.readLine();
      if(line.isNull() || line.startsWith("//-->"))
        break;
      script.append(line);
      script.append('\n');
      }

    // Add this block
    m_ScriptBlocks.append(script);
    }

  // Close file
  file.close();

  // Create and run the thread
  TestWorker *worker = new TestWorker(this, m_ScriptBlocks, m_ScriptEngine);
  connect(worker, &TestWorker::finished, worker, &QObject::deleteLater);
  worker->start();

  return SUCCESS;

 /*
  // Run the script
  // TODO: do this on a timer, one line at a time
  QJSValue rc = m_ScriptEngine->evaluate(script, fn.fileName());
  if(rc.isError())
    qDebug() << "JavaScript exception:" << rc.toString();

  return (rc.toInt() == 0) ? SUCCESS : REGRESSION_TEST_FAILURE;
  */
}

QObject *SNAPTestQt::findChild(QObject *parent, QString child)
{
  return parent->findChild<QObject *>(child);
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

void SNAPTestQt::print(QString text)
{
  qDebug() << text;
}

void SNAPTestQt::printChildrenRecursive(QObject *parent, QString offset)
{
  QString line = QString("%1%2 : %3").arg(offset,parent->metaObject()->className(),parent->objectName());
  qDebug() << line;

  foreach (QObject* child, parent->children())
    {
    QWidget *widget = dynamic_cast<QWidget *>(child);
    if(widget)
      printChildrenRecursive(child, offset + "  ");
    }

}

void SNAPTestQt::printChildren(QObject *parent)
{
  printChildrenRecursive(parent, "");
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
    QCoreApplication::exit(REGRESSION_TEST_FAILURE);
    }

}

SNAPTestQt::ReturnCode
SNAPTestQt::ListTests()
{
  cout << "Available Tests" << endl;
  cout << "  SnakeThreshQt    : Test segmentation with thresholding option" << endl;
  return SUCCESS;
}


TestWorker::TestWorker(QObject *parent, QStringList script, QJSEngine *engine)
  : QThread(parent)
{
  m_Script = script;
  m_Engine = engine;
}

void TestWorker::run()
{
  for(int i = 0; i < m_Script.size(); i++)
    {
    msleep(1000);

    QJSValue rc = m_Engine->evaluate(m_Script[i]);
    if(rc.isError())
      {
      qWarning() << "JavaScript exception:" << rc.toString();
      QCoreApplication::exit(SNAPTestQt::REGRESSION_TEST_FAILURE);
      }
    }

  // Once the test has completed, we can exit the application
  QCoreApplication::exit(SNAPTestQt::SUCCESS);
}
