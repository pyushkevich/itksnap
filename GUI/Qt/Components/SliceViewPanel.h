#ifndef SLICEVIEWPANEL_H
#define SLICEVIEWPANEL_H

#include <SNAPComponent.h>
#include <GlobalState.h>

class QMenu;
class QtInteractionDelegateWidget;
class SnakeModeRenderer;
class SliceWindowDecorationRenderer;
class DeformationGridRenderer;
class GenericSliceModel;
class QCursor;
class QToolButton;

class SliceWindowDecorationRenderer;

class CrosshairsInteractionMode;
class ThumbnailInteractionMode;
class PolygonDrawingInteractionMode;
class SnakeROIInteractionMode;
class PaintbrushInteractionMode;
class AnnotationInteractionMode;
class RegistrationInteractionMode;

class SliceRendererDelegate;
class GenericSliceRenderer;
class CrosshairsRenderer;
class PolygonDrawingRenderer;
class PaintbrushRenderer;
class AnnotationRenderer;
class DeformationGridRenderer;
class SnakeModeRenderer;
class SnakeROIRenderer;
class RegistrationRenderer;
class QtViewportReporter;
class SliceViewDelegateChain;

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

private:
  Ui::SliceViewPanel *ui;

  // Popup menus used for polygon operations
  QMenu *m_MenuPolyInactive, *m_MenuPolyEditing, *m_MenuPolyDrawing;

  // Global UI pointer
  GlobalUIModel *m_GlobalUI;

  // Slice model
  GenericSliceModel *m_SliceModel;

  // Custom cursor for drawing operations
  QCursor *m_DrawingCrosshairCursor;

  // Context menu tool button
  QToolButton *m_ContextToolButton;

  // Main renderer - owns this
  SmartPtr<GenericSliceRenderer> m_Renderer;

  // Canvas on which we are rendering
  QWidget *m_RendererCanvas;

  // Collection of interaction modes
  SliceViewDelegateChain *m_DelegateChain;

  // A size reporter for the area being painted by the renderers
  SmartPtr<QtViewportReporter> m_ViewportReporter;

  // Index of the panel
  unsigned int m_Index;
  void SetActiveMode(QWidget *mode, bool clearChildren = true);

  /** Update the expand view / contract view button based on the state */
  void UpdateExpandViewButton();

};

#endif // SLICEVIEWPANEL_H
