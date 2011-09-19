#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <QWidget>
#include <GlobalState.h>

class GenericSliceView;
class GlobalUIModel;

namespace Ui {
    class SliceViewPanel;
}

class SliceViewPanel : public QWidget
{
  Q_OBJECT

public:
  explicit SliceViewPanel(QWidget *parent = 0);
  ~SliceViewPanel();

  // Register the data model with this widget
  void Initialize(GlobalUIModel *model, unsigned int index);

  GenericSliceView *GetSliceView();

  // Callback for cursor change events
  void OnCursorUpdate();
  void OnImageDimensionsUpdate();

  void UpdateSlicePositionWidgets();

  // Callback for when the toolbar changes
  void OnToolbarModeChange();

private slots:
  void on_inSlicePosition_valueChanged(int value);

  void on_btnZoomToFit_clicked();

private:
  Ui::SliceViewPanel *ui;

  // Global UI pointer
  GlobalUIModel *m_GlobalUI;

  // Index of the panel
  unsigned int m_Index;
};

#endif // SLICEVIEWPANEL_H
