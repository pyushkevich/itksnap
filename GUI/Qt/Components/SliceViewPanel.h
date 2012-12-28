#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <SNAPComponent.h>
#include <GlobalState.h>

class GenericSliceView;
class QMenu;
class QtInteractionDelegateWidget;
class SnakeModeRenderer;

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
  void SetMouseMotionTracking(bool enable);


private slots:
  void on_inSlicePosition_valueChanged(int value);

  void on_btnZoomToFit_clicked();

  void onModelUpdate(const EventBucket &eb);

  void OnToolbarModeChange();

  void onContextMenu();

  void on_btnExpand_clicked();

private:
  Ui::SliceViewPanel *ui;

  // Popup menus used for polygon operations
  QMenu *m_MenuPolyInactive, *m_MenuPolyEditing, *m_MenuPolyDrawing;

  // Current event filter on the crosshair widget
  QWidget *m_CurrentEventFilter;

  // Global UI pointer
  GlobalUIModel *m_GlobalUI;

  // Some renderers don't require a separate widget (no user interaction)
  // and so they are owned by this panel.
  SmartPtr<SnakeModeRenderer> m_SnakeModeRenderer;


  // Index of the panel
  unsigned int m_Index;

  void SetActiveMode(QWidget *mode, bool clearChildren = true);

  /**
  The common setup is to have an event filter chain
    widget -- crosshair -- active_mode -- thumbnail
  In other words, all events first go to the thumbnail,
  then to the active mode, then to the crosshair mode
  */
  void ConfigureEventChain(QWidget *w);

  /**
   * Listen to mouse enter/exit events in order to show and hide toolbars
   */
  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

  /** Update the expand view / contract view button based on the state */
  void UpdateExpandViewButton();

};

#endif // SLICEVIEWPANEL_H
