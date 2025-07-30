#ifndef SNAPQTCOMMONTRANSLATIONS_H
#define SNAPQTCOMMONTRANSLATIONS_H

#include <QObject>
#include <SNAPCommon.h>
#include <ColorMap.h>

class SNAPQtCommonTranslations : public QObject
{
  Q_OBJECT
public:
  static QString translate(ImageIODisplayName name);
};

#endif // SNAPQTCOMMONTRANSLATIONS_H
