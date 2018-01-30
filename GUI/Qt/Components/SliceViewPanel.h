#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <SNAPComponent.h>
#include <GlobalState.h>

class GenericSliceView;
class QMenu;
class QtInteractionDelegateWidget;
class SnakeModeRenderer;
class SliceWindowDecorationRenderer;
class GenericSliceModel;
class QCursor;
class QToolButton;

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

  // Get the index of this panel
  irisGetMacro(Index, unsigned int)

  GenericSliceView *GetSliceView();

  // Callback for when the toolbar changes
  void SetMouseMotionTracking(bool enable);


private slots:
  void on_inSlicePosition_valueChanged(int value);

  void on_btnZoomToFit_clicked();

  void onModelUpdate(const EventBucket &eb);

  void OnToolbarModeChange();

  void onContextMenu();

  void on_btnExpand_clicked();

  void on_btnScreenshot_clicked();

  void on_btnToggleLayout_clicked();

  void on_actionZoom_In_triggered();

  void on_actionZoom_Out_triggered();

  void OnHoveredLayerChange(const EventBucket &eb);

  void on_actionAnnotationAcceptLine_triggered();

  void on_actionAnnotationClearLine_triggered();

  void on_actionAnnotationSelectAll_triggered();

  void on_actionAnnotationDelete_triggered();

  void on_actionAnnotationNext_triggered();

  void on_actionAnnotationPrevious_triggered();

private:
  Ui::SliceViewPanel *ui;

  // Popup menus used for polygon operations
  QMenu *m_MenuPolyInactive, *m_MenuPolyEditing, *m_MenuPolyDrawing;

  // Current event filter on the crosshair widget
  QWidget *m_CurrentEventFilter;

  // Global UI pointer
  GlobalUIModel *m_GlobalUI;

  // Slice model
  GenericSliceModel *m_SliceModel;

  // Custom cursor for drawing operations
  QCursor *m_DrawingCrosshairCursor;

  // Context menu tool button
  QToolButton *m_ContextToolButton;

  // Some renderers don't require a separate widget (no user interaction)
  // and so they are owned by this panel.
  SmartPtr<SnakeModeRenderer> m_SnakeModeRenderer;
  SmartPtr<SliceWindowDecorationRenderer> m_DecorationRenderer;

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
