#include <QScriptEngine>
#include <QTimer>
#include <QtGlobal>
#include<QDebug>

#include "MainImageWindow.h"
#include "IRISMainToolbox.h"
#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

#include "QtScriptTest1.h"

using namespace std;

QtScriptTest1::QtScriptTest1()
{
  m_pQTimer = new QTimer(this);
  m_pQTimer->setSingleShot(true);
}

QtScriptTest1::~QtScriptTest1()
{
  delete m_pQTimer;
}

void QtScriptTest1::Initialize(
  MainImageWindow *win,
  GlobalUIModel *model,
  std::string datadir)
{
  m_Window = win;
  m_Model = model;
  m_DataDir = datadir;

  // TODO: Check the data directory

}

void QtScriptTest1::Run( QScriptEngine * apEngine)
{
  m_pEngine = apEngine;

  connect(m_pQTimer, SIGNAL(timeout()), this, SLOT(RunQTimerTriggered()));
  m_pQTimer->start(1000);


}

void QtScriptTest1::RunQTimerTriggered()
{
  cout << "!!!!!!!!!!!!!!!!!!!!" << endl;


  QString commandLoad("snap.LoadRecent(");
  commandLoad += '"';
  commandLoad += "/Users/octavian/Programs/ITK-SNAP/Data/MRI-crop/MRIcrop-orig.gipl";
  commandLoad += '"';
  commandLoad += ");\n";

  QScriptValue res = m_pEngine->evaluate(commandLoad);

  IRISMainToolbox * pIRISMainToolbox = m_Window->m_Toolbox;
  CursorInspector * pCursorInspector = pIRISMainToolbox->m_CursorInspector;
  Ui::CursorInspector * pUiCursorInspector = pCursorInspector->ui;

  CursorInspectionModel * pCursorInspectionModel = m_Model->GetCursorInspectionModel();

  QScriptValue qscvalInCursorX = m_pEngine->newQObject(pUiCursorInspector->inCursorX);

  pUiCursorInspector->inCursorX->setValue(21);

  m_pEngine->globalObject().setProperty("inCursorX", qscvalInCursorX);

  QString commandSetX("inCursorX.valueChanged(21);\n");

  QString program = commandLoad// + commandSetX
          ;

  res = m_pEngine->evaluate(commandSetX);

  if (m_pEngine->hasUncaughtException()) {
      int line = m_pEngine->uncaughtExceptionLineNumber();
      qDebug() << "uncaught exception at line" << line << ":" << res.toString();
  }

  Vector3ui value;
  value[0] = 21;
  value[1] = 52;
  value[2] = 33;
  m_Model->SetCursorPosition(value);

  int nRes = pUiCursorInspector->inCursorX->value();

  int nReadRes = pUiCursorInspector->outLabelId->value();
  //This function just prints the results of the interogated image.
  //The check of correctness is done in CMakeLists
  cout << "QtScriptTest1::Run: " << nReadRes << endl;


  //IRISApplication *app = m_Model->GetDriver();
  //app->SetCursorPosition(value);

  /*
  QScriptValue buttonX = interpretor.newQObject();
  interpretor.globalObject().setProperty("", buttonX);
  QScriptValue
  */
  //return(0);

}
