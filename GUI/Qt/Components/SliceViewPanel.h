#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <SNAPComponent.h>
#include <GlobalState.h>

class GenericSliceView;
class QMenu;
class QtInteractionDelegateWidget;
class SnakeModeRenderer;
class SliceWindowDecorationRenderer;
class DeformationGridRenderer;
class GenericSliceModel;
class QCursor;
class QToolButton;

class SliceWindowDecorationNewRenderer;

class CrosshairsInteractionMode;
class ThumbnailInteractionMode;
class PolygonDrawingInteractionMode;
class SnakeROIInteractionMode;
class PaintbrushInteractionMode;
class AnnotationInteractionMode;
class RegistrationInteractionMode;

class GenericSliceNewRenderer;
class CrosshairsNewRenderer;
class PolygonDrawingNewRenderer;
class PaintbrushNewRenderer;
class AnnotationNewRenderer;
class DeformationGridNewRenderer;
class SnakeModeNewRenderer;
class SnakeROINewRenderer;
class RegistrationNewRenderer;
class QtViewportReporter;

namespace Ui {
    class SliceViewPanel;
}

class SliceViewPanelInteractionModes : public QObject
{
  friend class SliceViewPanel;
  Q_OBJECT

public:
  SliceViewPanelInteractionModes(QWidget *sliceView, QWidget *canvasWidget);

  void ConfigureEventChain(ToolbarModeType mode);

  void SetModel(GlobalUIModel *model, unsigned int index);

private:
  // Client widget, widget that actually is rendered on
  QWidget *m_SliceView, *m_CanvasWidget;

  // Interaction modes
  CrosshairsInteractionMode *m_CrosshairsMode, *m_ZoomPanMode;
  ThumbnailInteractionMode *m_ThumbnailMode;
  PolygonDrawingInteractionMode *m_PolygonMode;
  SnakeROIInteractionMode *m_SnakeROIMode;
  PaintbrushInteractionMode *m_PaintbrushMode;
  AnnotationInteractionMode *m_AnnotationMode;
  RegistrationInteractionMode *m_RegistrationMode;

  // Map from toolbar mode to interaction mode
  std::map<ToolbarModeType, QWidget *> m_ToolbarModeMap;

  // All modes including those that don't correspond to a toolbar mode
  std::list<QWidget *> m_AllModes;
};

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

  // Save screenshot to file
  void SaveScreenshot(const std::string &filename);


private slots:
  void on_actionAnnotationEdit_triggered();

private slots:
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

  void on_btnChangeViewer_clicked(bool checked);

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


  // Main renderer - owns this
  SmartPtr<GenericSliceNewRenderer> m_NewRenderer;

  // Canvas on which we are rendering
  QWidget *m_NewRendererCanvas;

  // Some renderers don't require a separate widget (no user interaction)
  // and so they are owned by this panel.
  SmartPtr<SnakeModeRenderer> m_SnakeModeRenderer;
  SmartPtr<SliceWindowDecorationRenderer> m_DecorationRenderer;
  SmartPtr<DeformationGridRenderer> m_DeformationGridRenderer;

  // Renderers
  SmartPtr<CrosshairsNewRenderer> m_CrosshairsNewRenderer;
  SmartPtr<PolygonDrawingNewRenderer> m_PolygonDrawingNewRenderer;
  SmartPtr<PaintbrushNewRenderer> m_PaintbrushNewRenderer;
  SmartPtr<AnnotationNewRenderer> m_AnnotationNewRenderer;
  SmartPtr<DeformationGridNewRenderer> m_DeformationGridNewRenderer;
  SmartPtr<SnakeModeNewRenderer> m_SnakeModeNewRenderer;
  SmartPtr<SnakeROINewRenderer> m_SnakeROINewRenderer;
  SmartPtr<RegistrationNewRenderer> m_RegistrationNewRenderer;

  // Some renderers don't require a separate widget (no user interaction)
  // and so they are owned by this panel.
  SmartPtr<SliceWindowDecorationNewRenderer> m_DecorationNewRenderer;

  // Collection of interaction modes
  SliceViewPanelInteractionModes *m_InteractionModes, *m_InteractionModesNew;

  // A size reporter for the area being painted by the renderers
  SmartPtr<QtViewportReporter> m_ViewportReporter;

  // Index of the panel
  unsigned int m_Index;

  void SetActiveMode(QWidget *mode, bool clearChildren = true);


  /**
   * Listen to mouse enter/exit events in order to show and hide toolbars
   */
  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

  /** Update the expand view / contract view button based on the state */
  void UpdateExpandViewButton();

};

#endif // SLICEVIEWPANEL_H
