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
    UIF_EDITING_MODE
  };


  FIRES(StateMachineChangeEvent)

  irisGetMacro(Parent, GenericSliceModel *)

  void SetParent(GenericSliceModel *model);

  /** Get the line drawing state */
  irisGetMacro(FlagDrawingLine, bool)

  /** Get the current line */
  irisGetMacro(CurrentLine, const LineSegment &)

  /** Get the physical length of current line */
  double GetCurrentLineLength();

  /** Compute angle between two lines */
  double GetAngleWithCurrentLine(const annot::LineSegmentAnnotation *lsa);

  /** Helper function - get current annotation mode from GlobalState */
  AnnotationMode GetAnnotationMode() const;

  /** Helper function - get annotation data from IRISApplication */
  ImageAnnotationData *GetAnnotations();

  /** Test if an annotation is visible in this slice */
  bool IsAnnotationVisible(const AbstractAnnotation *annot);


  bool ProcessPushEvent(const Vector3d &xSlice, bool shift_mod);

  bool ProcessDragEvent(const Vector3d &xSlice, bool shift_mod);

  bool ProcessReleaseEvent(const Vector3d &xSlice, bool shift_mod);

  bool IsDrawingRuler();

  void AcceptLine();

  void CancelLine();

  void SelectAllOnSlice();

  void DeleteSelectedOnSlice();

  bool CheckState(UIState state);


  Vector3f GetAnnotationCenter(const AbstractAnnotation *annot);

protected:

  AnnotationModel();
  virtual ~AnnotationModel();

  // Parent model
  GenericSliceModel *m_Parent;

  // Current state for line drawing
  bool m_FlagDrawingLine;
  LineSegment m_CurrentLine;

  // Motion-related variables
  Vector3f m_DragStart, m_DragLast;
  bool m_MovingSelection;

  double GetDistanceToLine(LineSegment &line, const Vector3d &point);
  double GetPixelDistanceToAnnotation(const AbstractAnnotation *annot, const Vector3d &point);
  void AdjustAngleToRoundDegree(LineSegment &ls, int n_degrees);
};

#endif // ANNOTATIONMODEL_H
