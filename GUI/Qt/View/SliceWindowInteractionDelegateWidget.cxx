#include "SliceWindowInteractionDelegateWidget.h"
#include "GenericSliceModel.h"
#include "GenericSliceView.h"
#include "GlobalUIModel.h"

SliceWindowInteractionDelegateWidget
::SliceWindowInteractionDelegateWidget(GenericSliceView *parent)
  : QtInteractionDelegateWidget(parent)
{
  m_ParentModel = NULL;
  m_ParentView = parent;
  m_LastPressLayoutCell.fill(0);
}

void SliceWindowInteractionDelegateWidget::preprocessEvent(QEvent *ev)
{
  // Do the parent's processing
  QtInteractionDelegateWidget::preprocessEvent(ev);

  // Deal with mouse events
  if(ev->type() == QEvent::MouseButtonPress ||
     ev->type() == QEvent::MouseButtonRelease ||
     ev->type() == QEvent::MouseMove ||
     ev->type() == QEvent::MouseButtonDblClick)
    {
    // Compute the spatial location of the event
    m_XSlice = to_double(m_ParentModel->MapWindowToSlice(
                           to_float(Vector2d(m_XSpace.extract(2)))));

    // If a mouse press, back up this info for drag tracking
    if(ev->type() == QEvent::MouseButtonPress)
      {
      m_LastPressXSlice = m_XSlice;
      }
    }
}

Vector3d
SliceWindowInteractionDelegateWidget
::GetEventWorldCoordinates(QMouseEvent *ev, bool flipY)
{
  // Make the parent window the current context
  QtAbstractOpenGLBox *parent = this->GetParentGLWidget();
  parent->makeCurrent();

  // Get the x,y coordinates of the event
  int x = ev->x();
  int y = (flipY) ? parent->height() - 1 - ev->y() : ev->y();

  // Get the cell size and the number of cells
  Vector2ui sz = m_ParentModel->GetSize();
  Vector2ui cells =
      m_ParentModel->GetParentUI()->GetSliceViewCellLayoutModel()->GetValue();
  int nrows = cells[0], ncols = cells[1];
  int icol, irow;

  // Determine which layout cell generated the event, unless we are dragging
  // in which case we want to use the last cell
  if(!this->isDragging())
    {
    // Which cell did the event fall in?
    icol = ev->x() / sz[0];
    irow = ev->y() / sz[1];

    // Ensure the column and row are valid, otherwise default to the first cell
    if(icol < 0 || icol >= ncols || irow < 0 || irow > nrows)
      {
      icol = 0;
      irow = 0;
      }

    // Store this for the next time
    m_LastPressLayoutCell = Vector2ui(irow, icol);
    }
  else
    {
    irow = m_LastPressLayoutCell[0];
    icol = m_LastPressLayoutCell[1];
    }

  // Convert the event coordinates into the model view coordinates
  double modelMatrix[16], projMatrix[16];
  GLint viewport[] = { icol * sz[0], (nrows - 1 - irow) * sz[1], sz[0], sz[1] };
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

  // Unproject to get the coordinate of the event
  Vector3d xProjection;
  gluUnProject(x, y, 0,
               modelMatrix,projMatrix,viewport,
               &xProjection[0], &xProjection[1], &xProjection[2]);

  return xProjection;
}
