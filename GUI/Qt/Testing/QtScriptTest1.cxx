#include <QScriptEngine>
#include <QTimer>
#include <QtGlobal>
#include<QDebug>

#include "MainImageWindow.h"
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
  commandLoad += "/Users/pauly/tk/itksnap_old/data/MRIcrop-orig.gipl";
  commandLoad += '"';
  commandLoad += ");\n";

  // two options to load data for the test are
  // 1. InsightSNAP -g image.nii --testQtScript ... (easy way)
  // 2. File->Open Image -> interact with fields in the IO dialog (is it possible?)


  // C++
  // sleep
  // run next line from a QtScript
  // ...

  // All this stuff below is in QtScript, not C++
  // SCRIPT FUNCTION LoadImage {
  //   timer fires
  //   m_Window->findChild("OpenImageAction") - call slot trigger()
  //   sleep
  //   m_FileDialog->findChild("InFileName") - call slot setText()
  //   sleep
  //   m_FileDialog->findChild("nextButton") - call slot trigger()
  // }

  // sleep
  // m_Window->findChild("InCursorX") - call slot setValue()

  CursorInspector *ci = m_Window->findChild<CursorInspector *>("CursorInspector");
  QSpinBox *inX = ci->findChild<QSpinBox *>("inCursorX");
  QSpinBox *outId = ci->findChild<QSpinBox *>("outLabelId");
  inX->setValue(21);


  // How does this work??
  QScriptValue res = m_pEngine->evaluate(commandLoad);

  /*
  MainControlPanel * pIRISMainToolbox = m_Window->m_ControlPanel;
  CursorInspector * pCursorInspector = pIRISMainToolbox->m_CursorInspector;
  Ui::CursorInspector * pUiCursorInspector = pCursorInspector->ui;

  CursorInspectionModel * pCursorInspectionModel = m_Model->GetCursorInspectionModel();


  pUiCursorInspector->inCursorX->setValue(21);
  */

  // Each widget has a widget name! You can search for a widget by its name
  //   pUiCursorInspector->inCursorX->objectName()
  //   something like ... m_Window->findChild("inCursorX")

  QScriptValue qscvalInCursorX = m_pEngine->newQObject(inX);
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
  m_Model->GetDriver()->SetCursorPosition(value);

  // int nRes = inX->value();

  int nReadRes = outId->value();
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
