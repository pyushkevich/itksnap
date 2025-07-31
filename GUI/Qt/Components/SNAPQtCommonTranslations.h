#ifndef SNAPQTCOMMONTRANSLATIONS_H
#define SNAPQTCOMMONTRANSLATIONS_H

#include "GlobalState.h"
#include <QObject>
#include <SNAPCommon.h>
#include <ColorMap.h>

class SNAPQtCommonTranslations : public QObject
{
  Q_OBJECT
public:
  static QString translate(ImageIODisplayName name);
  static QString translate(PreprocessingMode mode);
};

#endif // SNAPQTCOMMONTRANSLATIONS_H
