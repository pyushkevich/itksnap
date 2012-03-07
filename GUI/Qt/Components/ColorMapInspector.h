#ifndef COLORMAPINSPECTOR_H
#define COLORMAPINSPECTOR_H

#include "SNAPComponent.h"

class QtViewportReporter;
class ColorMapModel;

namespace Ui {
    class ColorMapInspector;
}

class ColorMapInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ColorMapInspector(QWidget *parent = 0);
  ~ColorMapInspector();

  void SetModel(ColorMapModel *model);

private slots:

  // Slot for model updates
  void onModelUpdate(const EventBucket &b);

private:
  Ui::ColorMapInspector *ui;

  // Model object
  ColorMapModel *m_Model;

  // Viewport reporter for the curve box
  QtViewportReporter *m_ColorMapBoxViewportReporter;
};

#endif // COLORMAPINSPECTOR_H
