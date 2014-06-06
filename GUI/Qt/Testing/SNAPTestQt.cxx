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

SNAPTestQt::ReturnCode
SNAPTestQt::RunTest(std::string test)
{
  if(test == "list")
    return ListTests();

  // Find the script file corresponding to the test
  QFile file(QString(":/scripts/Scripts/test_%1.js").arg(from_utf8(test)));
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

  // Start timer
  m_Timer.start(1000);

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

void SNAPTestQt::print(QString text)
{
  qDebug() << text;
}

void SNAPTestQt::onTimeout()
{
  // Get the next block
  QString script = m_ScriptBlocks.front();
  m_ScriptBlocks.pop_front();

  // Execute the script
  if(!script.isNull())
    {
    QJSValue rc = m_ScriptEngine->evaluate(script);
    if(rc.isError())
      {
      qDebug() << "JavaScript exception:" << rc.toString();
      m_ScriptBlocks.clear();
      }
    }
  else
    {
    qDebug() << "Script is finished!";
    }

  // Check timer
  if(!m_ScriptBlocks.size())
    m_Timer.stop();
}

SNAPTestQt::ReturnCode
SNAPTestQt::ListTests()
{
  cout << "Available Tests" << endl;
  cout << "  SnakeThreshQt    : Test segmentation with thresholding option" << endl;
  return SUCCESS;
}
