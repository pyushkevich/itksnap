#include <QScriptEngine>

#include "MainImageWindow.h"
#include "IRISMainToolbox.h"
#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

#include "QtScriptTest1.h"

using namespace std;

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
  QString commandLoad("snap.LoadRecent(");
  commandLoad += '"';
  commandLoad += "/Users/octavian/Programs/ITK-SNAP/Data/MRI-crop/MRIcrop-orig.gipl";
  commandLoad += '"';
  commandLoad += ");\n";

  //QScriptValue res = apEngine->evaluate(commandLoad);

  IRISMainToolbox * pIRISMainToolbox = m_Window->m_Toolbox;
  CursorInspector * pCursorInspector = pIRISMainToolbox->m_CursorInspector;
  Ui::CursorInspector * pUiCursorInspector = pCursorInspector->ui;

  CursorInspectionModel * pCursorInspectionModel = m_Model->GetCursorInspectionModel();

  QScriptValue qscvalInCursorX = apEngine->newQObject(pUiCursorInspector->inCursorX);

  pUiCursorInspector->inCursorX->setValue(21);

  apEngine->globalObject().setProperty("inCursorX", qscvalInCursorX);

  QString commandSetX("inCursorX.valueChanged(21);\n");

  QString program = commandLoad// + commandSetX
          ;

  QScriptValue res = apEngine->evaluate(program);

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
