#ifndef PAINTBRUSHMODEL_H
#define PAINTBRUSHMODEL_H

#include "AbstractModel.h"
#include "GlobalState.h"
#include <vtkSmartPointer.h>
#include <vtkPoints2D.h>

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
  bool ProcessDragEvent(const Vector3d &xSlice,
                        const Vector3d &xSliceLast,
                        double          pixelsMoved,
                        bool            release);

  bool ProcessMouseMoveEvent(const Vector3d &xSlice);
  bool ProcessMouseLeaveEvent();

  void AcceptAtCursor();

  // Get the location in slice coordinates where the center of the paintbrush
  // should be rendered
  Vector3d GetCenterOfPaintbrushInSliceSpace();

  // Get whether main image has been transformed
  bool HasMainImageTransformed();

  bool TestInside(const Vector2d &x, const PaintbrushSettings &ps);
  bool TestInside(const Vector3d &x, const PaintbrushSettings &ps);

  /**
   * The outline of the brush in slice coordinates, relative to the center of
   * the brush (GetCenterOfPaintbrushInSliceSpace). It is computed from the
   * brush settings and the reference space spacing, and recomputed when these
   * change. It is drawn by the renderer, and painted when the main image is
   * transformed (non-orthogonal slicing).
   */
  const std::vector<Vector2d> &GetBrushOutline();

protected:
  // Whether we are inverting the paintbrush when drawing
  bool m_ReverseMode;

  // Mouse position in voxel coordinates
  Vector3i  m_MousePosition;
  bool      m_MouseInside;

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
  void     ComputeMousePosition(const Vector3d &xSlice);

  bool ApplyBrush(bool reverse_mode, bool dragging, bool release);
  void CommitDrawing();

  // Get the effective paintbrush settings, which may differ from the global ones
  // depending on deep learning mode
  PaintbrushSettings GetEffectivePaintbrushSettings();

  GenericSliceModel      *m_Parent;
  BrushWatershedPipeline *m_Watershed;

  // The brush outline, and the settings and spacing it was computed for
  std::vector<Vector2d> m_BrushOutline;
  PaintbrushSettings    m_BrushOutlineSettings;
  Vector3d              m_BrushOutlineSpacing;

  // Append a vertex to the outline, skipping duplicate and collinear points
  void InsertWithCollinearCheck(const Vector2d &head);

  // Apply brush logic if main is transformed
  // Returns true if changes were made
  bool ApplyBrushByPolygonRasterization(bool reverse_mode, bool dragging);

  friend class PaintbrushRenderer;

};

#endif // PAINTBRUSHMODEL_H
