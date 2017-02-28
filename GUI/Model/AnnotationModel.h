#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include "SNAPCommon.h"
#include "IRISException.h"
#include "GlobalState.h"
#include "ImageAnnotationData.h"

#include <list>
#include <utility>
#include <set>

class GenericSliceModel;


/**
 * @brief The UI model for slice annotation
 */
class AnnotationModel : public AbstractModel
{
public:

  typedef annot::AbstractAnnotation AbstractAnnotation;
  typedef annot::LineSegmentAnnotation LineSegmentAnnotation;
  typedef annot::LandmarkAnnotation LandmarkAnnotation;

  typedef annot::LineSegment LineSegment;

  irisITKObjectMacro(AnnotationModel, AbstractModel)

  // States
  enum UIState {
    UIF_LINE_MODE,
    UIF_LINE_MODE_DRAWING,
    UIF_LANDMARK_MODE,
    UIF_EDITING_MODE
  };


  FIRES(StateMachineChangeEvent)

  irisGetMacro(Parent, GenericSliceModel *)

  void SetParent(GenericSliceModel *model);

  /** Get the line drawing state */
  irisGetMacro(FlagDrawingLine, bool)

  /** Get the current line */
  irisGetMacro(CurrentLine, const LineSegment &)

  /** Whether we are moving something now */
  irisIsMacro(MovingSelection)

  /** Get the physical length of line segment */
  double GetLineLength(const Vector3d &xSliceA, const Vector3d &xSliceB);

  /** Get the physical length of current line */
  double GetCurrentLineLength();

  /** Get the length of the current line in logical (non-retina) screen pixel units */
  double GetCurrentLineLengthInPixels();

  /** Compute angle between two lines */
  double GetAngleWithCurrentLine(const annot::LineSegmentAnnotation *lsa);

  /** Helper function - get current annotation mode from GlobalState */
  AnnotationMode GetAnnotationMode() const;

  /** Helper function - get annotation data from IRISApplication */
  ImageAnnotationData *GetAnnotations();

  /** Test if an annotation is visible in this slice */
  bool IsAnnotationVisible(const AbstractAnnotation *annot);


  bool ProcessPushEvent(const Vector3d &xSlice, bool shift_mod);

  bool ProcessMoveEvent(const Vector3d &xSlice, bool shift_mod, bool drag);

  bool ProcessReleaseEvent(const Vector3d &xSlice, bool shift_mod);

  bool IsDrawingRuler();

  void AcceptLine();

  void CancelLine();

  void SelectAllOnSlice();

  void DeleteSelectedOnSlice();

  void GoToNextAnnotation();
  void GoToPreviousAnnotation();

  bool CheckState(UIState state);

  bool IsHoveringOverAnnotation(const Vector3d &xSlice);

  /** Set the text assigned to the current annotation */
  irisGetSetMacro(CurrentAnnotationText, std::string)

  Vector3d GetAnnotationCenter(const AbstractAnnotation *annot);

  void GetLandmarkArrowPoints(const annot::Landmark &lm, Vector3d &outHeadXSlice, Vector3d &outTailXSlice);



protected:

  AnnotationModel();
  virtual ~AnnotationModel();

  // Parent model
  GenericSliceModel *m_Parent;

  // Current state for line drawing
  bool m_FlagDrawingLine;
  LineSegment m_CurrentLine;

  // Motion-related variables
  Vector3d m_DragStart, m_DragLast;
  bool m_MovingSelection;
  int m_MovingSelectionHandle;
  annot::AbstractAnnotation *m_MovingSelectionHandleAnnot;

  // Text assigned to the currently drawn annotation
  std::string m_CurrentAnnotationText;

  double GetDistanceToLine(const Vector3d &x1, const Vector3d &x2, const Vector3d &point);
  double GetDistanceToLine(LineSegment &line, const Vector3d &point);
  double GetPixelDistanceToAnnotation(const AbstractAnnotation *annot, const Vector3d &point);
  void AdjustAngleToRoundDegree(LineSegment &ls, int n_degrees);
  AbstractAnnotation *GetAnnotationUnderCursor(const Vector3d &xSlice);
  void GoToNextOrPrevAnnotation(int direction);

  annot::AbstractAnnotation *GetSelectedHandleUnderCusror(const Vector3d &xSlice, int &out_handle);

  bool TestPointInClickRadius(const Vector3d &xClickSlice,
                              const Vector3d &xPointSlice,
                              int logical_pixels);

  // Apply move command to annotation handle
  void MoveAnnotationHandle(AbstractAnnotation *ann, int handle, const Vector3d &deltaPhys);
};

#endif // ANNOTATIONMODEL_H
