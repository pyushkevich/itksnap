#ifndef PAINTBRUSHMODEL_H
#define PAINTBRUSHMODEL_H

#include "AbstractModel.h"
#include "GlobalState.h"
#include "GenericImageData.h" //contains def. of LabelImageWrapper

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

  bool ProcessWheelEvent(int delta);
  bool ProcessPushEvent(const Vector3f &xSlice, bool reverse_mode);
  bool ProcessDragEvent(const Vector3f &xSlice, const Vector3f &xSliceLast,
                        double pixelsMoved, bool release);

  bool ProcessMouseMoveEvent(const Vector3f &xSlice);
  //bool ProcessMouseEnterEvent();
  bool ProcessMouseLeaveEvent();

  void AcceptAtCursor();

  // Get the location in slice coordinates where the center of the paintbrush
  // should be rendered
  Vector3f GetCenterOfPaintbrushInSliceSpace();


protected:

  // Whether we are inverting the paintbrush when drawing
  bool m_ReverseMode;

  // Mouse position in voxel coordinates
  Vector3ui m_MousePosition;
  bool m_MouseInside;

  //for WS updates
  double m_level;
  bool m_PrecomputedWS;
  Vector3f m_offset;
  LabelImageWrapper::ImageType::RegionType m_OldxTestRegion;

  // Mouse position in slice coordinates from which we need to draw the
  // next segment
  Vector3f m_LastApplyX;

  PaintbrushModel();
  virtual ~PaintbrushModel();

  Vector3f ComputeOffset();
  void ComputeMousePosition(const Vector3f &xSlice);

  bool ApplyBrush(bool reverse_mode, bool dragging);
  bool UpdateBrush();
  bool TestInside(const Vector2d &x, const PaintbrushSettings &ps);
  bool TestInside(const Vector3d &x, const PaintbrushSettings &ps);

  GenericSliceModel *m_Parent;
  BrushWatershedPipeline *m_Watershed;

  friend class PaintbrushRenderer;

};

#endif // PAINTBRUSHMODEL_H
