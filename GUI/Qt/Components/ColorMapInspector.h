#ifndef COLORMAPINSPECTOR_H
#define COLORMAPINSPECTOR_H

#include "SNAPCommon.h"
#include "SNAPComponent.h"
#include <map>

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

  void PromptUserForColor();

private slots:

  // Slot for model updates
  void onModelUpdate(const EventBucket &b);

  void on_btnControlColor_clicked();

  void on_inPreset_currentIndexChanged(int index);

  void on_btnAddPreset_clicked();

  void on_btnDelPreset_clicked();

  void on_btnDeleteControl_clicked();

private:

  void PopulatePresets();

  Ui::ColorMapInspector *ui;

  // Model object
  ColorMapModel *m_Model;

  // Viewport reporter for the curve box
  SmartPtr<QtViewportReporter> m_ColorMapBoxViewportReporter;

  // Icons for the different presets
  std::map<std::string, QIcon> m_PresetMap;

  // Whether an update is being done to the presets
  bool m_PresetsUpdating;
};

#endif // COLORMAPINSPECTOR_H
