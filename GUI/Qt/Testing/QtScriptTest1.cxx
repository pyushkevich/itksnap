#include <QScriptEngine>

#include "QtScriptTest1.h"

void QtScriptTest1( QScriptEngine * apEngine)
{
  QString commandLoad("snap.LoadRecent(");
  commandLoad += '"';
  commandLoad += "/Users/octavian/Programs/ITK-SNAP/Data/MRI-crop/MRIcrop-orig.gipl";
  commandLoad += '"';
  commandLoad += ")";

  QScriptValue res = apEngine->evaluate(commandLoad);

  /*
    QScriptValue buttonX = interpretor.newQObject();
    interpretor.globalObject().setProperty("", buttonX);
    QScriptValue
    */
  //return(0);

}
