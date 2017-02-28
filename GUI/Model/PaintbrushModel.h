#ifndef PAINTBRUSHMODEL_H
#define PAINTBRUSHMODEL_H

#include "AbstractModel.h"
#include "GlobalState.h"

class GenericSliceModel;
class BrushWatershedPipeline;


class PaintbrushModel : public AbstractModel
{
public:

  irisITKObjectMacro(PaintbrushModel, AbstractModel)

  itkEventMacro(PaintbrushMovedEvent, IRISEvent)

  irisGetSetMacro(Parent, GenericSliceModel *)

  irisIsMacro(MouseInside)

  FIRES(PaintbrushMovedEvent)

  bool ProcessPushEvent(const Vector3d &xSlice, const Vector2ui &gridCell, bool reverse_mode);
  bool ProcessDragEvent(const Vector3d &xSlice, const Vector3d &xSliceLast,
                        double pixelsMoved, bool release);

  bool ProcessMouseMoveEvent(const Vector3d &xSlice);
  bool ProcessMouseLeaveEvent();

  void AcceptAtCursor();

  // Get the location in slice coordinates where the center of the paintbrush
  // should be rendered
  Vector3d GetCenterOfPaintbrushInSliceSpace();


protected:

  // Whether we are inverting the paintbrush when drawing
  bool m_ReverseMode;

  // Mouse position in voxel coordinates
  Vector3ui m_MousePosition;
  bool m_MouseInside;

  // Mouse position in slice coordinates from which we need to draw the
  // next segment
  Vector3d m_LastApplyX;

  // Layer over which the drawing operation started
  unsigned long m_ContextLayerId;

  // Whether the push operation was in a paintable location
  bool m_IsEngaged;

  PaintbrushModel();
  virtual ~PaintbrushModel();

  Vector3d ComputeOffset();
  void ComputeMousePosition(const Vector3d &xSlice);

  bool ApplyBrush(bool reverse_mode, bool dragging);
  bool TestInside(const Vector2d &x, const PaintbrushSettings &ps);
  bool TestInside(const Vector3d &x, const PaintbrushSettings &ps);

  GenericSliceModel *m_Parent;
  BrushWatershedPipeline *m_Watershed;

  friend class PaintbrushRenderer;

};

#endif // PAINTBRUSHMODEL_H
