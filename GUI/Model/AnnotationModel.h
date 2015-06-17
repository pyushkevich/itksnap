#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include "SNAPCommon.h"
#include "IRISException.h"

#include <list>
#include <utility>

class GenericSliceModel;

/**
 * @brief The UI model for slice annotation
 */
class AnnotationModel : public AbstractModel
{
public:

  irisITKObjectMacro(AnnotationModel, AbstractModel)

  // States
  enum UIState {
    UIF_LINE_MODE,
    UIF_LINE_MODE_DRAWING,
    UIF_EDITING_MODE
  };


  FIRES(StateMachineChangeEvent)

  irisSetMacro(Parent, GenericSliceModel *)
  irisGetMacro(Parent, GenericSliceModel *)

  /** Possible interaction modes for the annotation tool */
  enum Mode {
    LINE_DRAWING, SELECT, MOVE
  };

  /** Data structures describing line annotations */
  typedef std::pair<Vector3f, Vector3f> LineIntervalType;
  typedef std::list<LineIntervalType> LineIntervalList;

  /** Set the current mode */
  void SetMode(Mode mode);

  /** Get the current mode */
  irisGetMacro(Mode, Mode)

  /** Get the line drawing state */
  irisGetMacro(FlagDrawingLine, bool)

  /** Get the current line */
  irisGetMacro(CurrentLine, const LineIntervalType &)

  /** Get the lines previously drawn */
  irisGetMacro(Lines, const LineIntervalList &)

  /** Get the physical length of current line */
  double GetCurrentLineLength();

  /** Compute angle between two lines */
  double GetAngleBetweenLines(const LineIntervalType &l1, const LineIntervalType &l2);

  bool ProcessPushEvent(const Vector3d &xSlice);

  bool ProcessDragEvent(const Vector3d &xSlice);

  bool ProcessReleaseEvent(const Vector3d &xSlice);

  void AcceptLine();

  bool CheckState(UIState state);


protected:

  AnnotationModel();
  virtual ~AnnotationModel();

  // Parent model
  GenericSliceModel *m_Parent;

  // Line intervals annotated by the user
  LineIntervalList m_Lines;

  // Current state for line drawing
  bool m_FlagDrawingLine;
  LineIntervalType m_CurrentLine;

  // Current mode
  Mode m_Mode;
};

#endif // ANNOTATIONMODEL_H
