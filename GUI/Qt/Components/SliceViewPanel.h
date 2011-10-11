#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <SNAPComponent.h>
#include <GlobalState.h>

class GenericSliceView;
class QMenu;
class QtInteractionDelegateWidget;

namespace Ui {
    class SliceViewPanel;
}

class SliceViewPanel : public SNAPComponent
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



private slots:
  void on_inSlicePosition_valueChanged(int value);

  void on_btnZoomToFit_clicked();

  void onModelUpdate(const EventBucket &eb);

  void OnToolbarModeChange();

  void onContextMenu();

private:
  Ui::SliceViewPanel *ui;

  // Popup menus used for polygon operations
  QMenu *m_MenuPolyInactive, *m_MenuPolyEditing, *m_MenuPolyDrawing;

  // Global UI pointer
  GlobalUIModel *m_GlobalUI;

  // Index of the panel
  unsigned int m_Index;

  void SetActiveMode(QWidget *mode, bool clearChildren = true);
};

#endif // SLICEVIEWPANEL_H
