#ifndef IMAGEINFOINSPECTOR_H
#define IMAGEINFOINSPECTOR_H

#include <SNAPComponent.h>
#include "SNAPCommon.h"

class ImageInfoModel;
class IntensityUnderCursorRenderer;
class QtViewportReporter;


namespace Ui {
class ImageInfoInspector;
}

class ImageInfoInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ImageInfoInspector(QWidget *parent = 0);
  ~ImageInfoInspector();

  void SetModel(ImageInfoModel *model);

private:

  ImageInfoModel *m_Model;

  Ui::ImageInfoInspector *ui;

  // Viewport reporter for the curve box
  SmartPtr<QtViewportReporter> m_PlotBoxViewportReporter;
  SmartPtr<IntensityUnderCursorRenderer> m_IntensityRenderer;

};

#endif // IMAGEINFOINSPECTOR_H
