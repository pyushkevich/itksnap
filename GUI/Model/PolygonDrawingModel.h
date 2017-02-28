#ifndef POLYGONDRAWINGMODEL_H
#define POLYGONDRAWINGMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include <SNAPCommon.h>
#include <IRISException.h>

namespace itk {
  template <class TPixel, unsigned int VDimensions> class Image;
}

class GenericSliceModel;


struct PolygonVertex
{
  double x, y;
  bool selected;
  bool control;
  PolygonVertex(double x_, double y_, bool on_, bool ctl_)
    : x(x_), y(y_), selected(on_), control(ctl_) {}
  PolygonVertex() : x(0.0), y(0.0), selected(false), control(true) {}
  double &operator[](unsigned int i)
  { return (i==0) ? x : y; }
};

enum PolygonDrawingUIState {
  UIF_INACTIVE,
  UIF_DRAWING,
  UIF_EDITING,
  UIF_HAVEPOLYGON,
  UIF_HAVECACHED,
  UIF_HAVE_VERTEX_SELECTION,
  UIF_HAVE_EDGE_SELECTION
};

class PolygonDrawingModel : public AbstractModel
{
public:

  irisITKObjectMacro(PolygonDrawingModel, AbstractModel)

  FIRES(StateMachineChangeEvent)

  irisSetMacro(Parent, GenericSliceModel *)
  irisGetMacro(Parent, GenericSliceModel *)

  /** Vertex structure */
  typedef PolygonVertex Vertex;
  typedef std::list<Vertex> VertexList;
  typedef VertexList::iterator VertexIterator;
  typedef VertexList::reverse_iterator VertexRIterator;

  typedef vnl_vector_fixed<double, 4> BoxType;

  /** States that the polygon drawing is in */
  enum PolygonState { INACTIVE_STATE = 0, DRAWING_STATE, EDITING_STATE };

  /** Render polygon onto a target image */
  void AcceptPolygon(std::vector<IRISWarning> &warnings);

  /** Copies cached polygon to current polygon */
  void PastePolygon(void);

  /** Deletes selected vertices */
  void Delete();

  /** Inserts vertices in selected edges */
  void Insert();

  /** Clears the drawing */
  void Reset();

  /** In drawing mode, remove the last point drawn */
  void DropLastPoint();

  /** In drawing mode, close the polygon (same as RMB) */
  void ClosePolygon();

  /** Can the polygon be closed? */
  bool CanClosePolygon();

  /** Can last point be dropped? */
  bool CanDropLastPoint();

  /** Can edges be split? */
  bool CanInsertVertices();

  /** Get the current state of the polygon editor */
  irisGetMacro(State, PolygonState)

  /** How many vertices are selected */
  irisGetMacro(SelectedVertices,bool)

  /** How many vertices are selected */
  irisGetMacro(CachedPolygon,bool)

  /** Set the accuracy of freehand curve fitting */
  irisRangedPropertyAccessMacro(FreehandFittingRate, double)

  /** Access to the vertices */
  irisGetMacro(Vertices, const VertexList &)
  irisSetMacro(Vertices, const VertexList &)

  /** Access to the vertices */
  irisGetMacro(DragVertices, const VertexList &)
  irisSetMacro(DragVertices, const VertexList &)

  /** Current selection box */
  irisGetMacro(SelectionBox, const BoxType &)

  /** Current edit box */
  irisGetMacro(EditBox, const BoxType &)

  /** Are we dragging the selection box? */
  irisIsMacro(DraggingPickBox)

  /** Is the cursor hovering over the starting voxel */
  irisIsMacro(HoverOverFirstVertex)

  bool ProcessPushEvent(double x, double y, bool shift_state);

  bool ProcessDragEvent(double x, double y);

  bool ProcessMouseMoveEvent(double x, double y);

  bool ProcessReleaseEvent(double x, double y);

  /**
   * Return the size of a screen logical pixel (as opposed to a physical
   * pixel on the retina screen) in slice coordinate units
   */
  Vector2d GetPixelSize();

  bool CheckState(PolygonDrawingUIState state);

protected:
  PolygonDrawingModel();
  virtual ~PolygonDrawingModel();

  GenericSliceModel *m_Parent;

  // Array of vertices, cached vertices from last operation

  VertexList m_Vertices, m_Cache;
  VertexList m_DragVertices;

  // State of the system
  PolygonState m_State;

  bool m_CachedPolygon;
  bool m_SelectedVertices;
  bool m_DraggingPickBox;

  // contains selected points
  BoxType m_EditBox;

  // box the user drags to select new points
  BoxType m_SelectionBox;

  double m_StartX, m_StartY;

  // Whether we are hovering over the starting vertex
  bool m_HoverOverFirstVertex;


  void ComputeEditBox();
  void Add(double x, double y, int selected);
  void ProcessFreehandCurve();

  bool CheckClickOnVertex(double x, double y,
                          double pixel_x, double pixel_y, int k);

  bool CheckClickOnLineSegment(double x, double y,
                               double pixel_x, double pixel_y, int k);

  // Check if the cursor (x,y) is near the first vertex (i.e., we should be
  // closing the polygon)
  bool CheckNearFirstVertex(double x, double y, double pixel_x, double pixel_y);

  int GetNumberOfSelectedSegments();

  void SetState(PolygonState state);

  // Freehand fitting rate
  SmartPtr<ConcreteRangedDoubleProperty> m_FreehandFittingRateModel;

  // Type definition for the slice used for polygon rendering
  typedef itk::Image<unsigned char,2> PolygonSliceType;
  typedef SmartPtr<PolygonSliceType> PolygonSlicePointer;

  /** Slice used for polygon drawing and merging */
  PolygonSlicePointer m_PolygonSlice;
};

#endif // POLYGONDRAWINGMODEL_H
