#include <QScriptEngine>
#include <QTimer>
#include <QtGlobal>
#include<QDebug>
#include<QThread>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QFile>


#include "ImageIOWizard.h"
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
  m_pQTimer->setSingleShot(false);
  m_pQTimer->setInterval(2000);
  m_pQTimer->start(2000);
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

  connect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_ActionOpenMainTriggered()));
  m_pQTimer->start(1000);

}

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};

void QtScriptTest1::Run_ActionOpenMainTriggered()
{
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

  QAction * pActionOpenMain = m_Window->findChild<QAction *>("actionOpenMain");
  disconnect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_ActionOpenMainTriggered()));
  m_pQTimer->start(1000);
  connect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_SetFileDetails()));

  pActionOpenMain->trigger();


}

void QtScriptTest1::Run_SetFileDetails()
{

  ImageIOWizard * m_pWiz = m_Window->findChild<ImageIOWizard *>();


  QWizardPage * pPage_File = m_pWiz->page ( ImageIOWizard::Page_File );

  QLineEdit * pInFilename = m_pWiz->findChild<QLineEdit *>("inFilename");
  pInFilename->setText("/Users/octavian/Programs/ITK-SNAP/Data/MRI-crop/MRIcrop-orig.gipl");
  QComboBox * pInFormat = m_pWiz->findChild<QComboBox *>("inFormat");
  pInFormat->setCurrentIndex(4);

  disconnect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_SetFileDetails()));
  //m_pQTimer->start(2000);
  //connect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_ValidateNext()));

   QApplication::processEvents();
   Run_ValidateNext();
}


void QtScriptTest1::Run_ValidateNext()
{

  ImageIOWizard * m_pWiz = m_Window->findChild<ImageIOWizard *>();

  m_pWiz->next();

  //QPushButton * pQPushButton = (QPushButton *)m_pWiz->button(QWizard::NextButton);
  //pQPushButton->click();

  disconnect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_ValidateNext()));
  m_pQTimer->stop();

  delete m_pQTimer;
  m_pQTimer = new QTimer(this);
  m_pQTimer->setSingleShot(false);
  connect(m_pQTimer, SIGNAL(timeout()), this, SLOT(Run_ValidateFinish));
  m_pQTimer->setInterval(2000);
  m_pQTimer->start(2000);

  QApplication::processEvents();
  Run_ValidateFinish();

}


void QtScriptTest1::Run_ValidateFinish()
{

  ImageIOWizard * m_pWiz = m_Window->findChild<ImageIOWizard *>();

  QPushButton * pQPushButton = (QPushButton *)m_pWiz->button(QWizard::FinishButton);
  pQPushButton->click();

  m_pQTimer->stop();

  QApplication::processEvents();

  /*
  QString commandLoad("snap.LoadRecent(");
  commandLoad += '"';
  commandLoad += "/Users/pauly/tk/itksnap_old/data/MRIcrop-orig.gipl";
  commandLoad += '"';
  commandLoad += ");\n";
  */

  /*
  QScriptValue qscvalActionOpenMain = m_pEngine->newQObject(pActionOpenMain);
  m_pEngine->globalObject().setProperty("actionOpenMain", qscvalInCursorX);

  QString commandActionOpenmain("inCursorX.valueChanged(21);\n");

  QString program = commandLoad// + commandSetX
          ;
*/

  // How does this work??
  //QScriptValue res = m_pEngine->evaluate(commandLoad);


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


  QSpinBox * inX = m_Window->findChild<QSpinBox *>("inCursorX");
  QSpinBox * inY = m_Window->findChild<QSpinBox *>("inCursorY");
  QSpinBox * inZ = m_Window->findChild<QSpinBox *>("inCursorZ");

  QScriptValue qscvalInCursorX = m_pEngine->newQObject(inX);
  QScriptValue qscvalInCursorY = m_pEngine->newQObject(inY);
  QScriptValue qscvalInCursorZ = m_pEngine->newQObject(inZ);

  m_pEngine->globalObject().setProperty("inCursorX", qscvalInCursorX);
  m_pEngine->globalObject().setProperty("inCursorY", qscvalInCursorY);
  m_pEngine->globalObject().setProperty("inCursorZ", qscvalInCursorZ);

  QFile file("/Users/octavian/Programs/ITK-SNAP/itksnap_qtsnap/QtCreator/itksnap/GUI/Qt/Testing/QtScriptTest1/ValidateFinish.js");
  if(!file.open(QIODevice::ReadOnly))
  {
      int indy = 1;
      exit(0);
  }

  QTextStream in (&file);
  in.setCodec("UTF-8");
  QString commandSetXYZ = in.readAll();
  file.close();

  QScriptValue res = m_pEngine->evaluate(commandSetXYZ);

  if (m_pEngine->hasUncaughtException()) {
      int line = m_pEngine->uncaughtExceptionLineNumber();
      qDebug() << "uncaught exception at line" << line << ":" << res.toString();
  }
  QApplication::processEvents();

  //QSpinBox *outId = ci->findChild<QSpinBox *>("outLabelId");
  QSpinBox *outId = m_Window->findChild<QSpinBox *>("outLabelId");
  int nVal = outId->value();
  cerr << "Result of point evaluation is: " << nVal << endl;

  QTableWidget *pTableWidget = m_Window->findChild<QTableWidget *>("tableVoxelUnderCursor");
  QTableWidgetItem *item = pTableWidget->item(0, 1);
  QString strVal = item->text();
  int nReadRes = strVal.toInt();
  cout << "QtScriptTest1::Run: " << nReadRes << endl;

  /*
  res = m_pEngine->evaluate(commandSetX);

  if (m_pEngine->hasUncaughtException()) {
      int line = m_pEngine->uncaughtExceptionLineNumber();
      qDebug() << "uncaught exception at line" << line << ":" << res.toString();
  }
  */

  //Tavi crashing so far ---- cout << "QtScriptTest1::Run: " << nReadRes << endl;

  exit(0);

}

////////
//The following code is kept for reference while trying to transform QtScriptTest
//into a as generic as possible code that answers to QtScript calls.
////////

void QtScriptTest1::Former_RunQTimerTriggered()
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
