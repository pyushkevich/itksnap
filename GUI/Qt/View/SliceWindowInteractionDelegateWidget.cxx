#include "SliceWindowInteractionDelegateWidget.h"
#include "GenericSliceModel.h"
#include "GenericSliceView.h"
#include "GlobalUIModel.h"
#include "DisplayLayoutModel.h"

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

  // Don't do nothing if slice not initialized
  if(!m_ParentModel || !m_ParentModel->IsSliceInitialized())
    return;

  // Deal with mouse events
  if(ev->type() == QEvent::MouseButtonPress ||
     ev->type() == QEvent::MouseButtonRelease ||
     ev->type() == QEvent::MouseMove ||
     ev->type() == QEvent::MouseButtonDblClick ||
     ev->type() == QEvent::ContextMenu)
    {
    // Compute the spatial location of the event
    m_XSlice = to_double(m_ParentModel->MapWindowToSlice(
                           to_float(Vector2d(m_XSpace.extract(2)))));

    // Determine whether the mouse is over a layer, and if so what layer it is, and
    // whether the layer is shown as a thumbnail or not
    QPoint pos;
    if(dynamic_cast<QMouseEvent *>(ev))
      pos = dynamic_cast<QMouseEvent *>(ev)->pos();
    else if(dynamic_cast<QContextMenuEvent *>(ev))
      pos = dynamic_cast<QContextMenuEvent *>(ev)->pos();

    Vector2i x(pos.x(),
               m_ParentModel->GetSizeReporter()->GetLogicalViewportSize()[1] - pos.y());

    // The hovered over layer is stored in m_HoverOverLayer
    m_HoverOverLayer =
        m_ParentModel->GetContextLayerAtPosition(x[0], x[1], m_HoverOverLayerIsThumbnail);
    }
}

void SliceWindowInteractionDelegateWidget::postprocessEvent(QEvent *ev)
{
  QtInteractionDelegateWidget::postprocessEvent(ev);
  if(ev->type() == QEvent::MouseButtonPress && ev->isAccepted())
    {
    m_LastPressXSlice = m_XSlice;
    }
  m_HoverOverLayer = NULL;
}

bool SliceWindowInteractionDelegateWidget::IsMouseOverLayerThumbnail()
{
  return m_HoverOverLayer != NULL && m_HoverOverLayerIsThumbnail;
}

bool SliceWindowInteractionDelegateWidget::IsMouseOverFullLayer()
{
  return m_HoverOverLayer != NULL && !m_HoverOverLayerIsThumbnail;
}



#include <QDebug>
#include <QWindow>

Vector3d
SliceWindowInteractionDelegateWidget
::GetEventWorldCoordinates(QMouseEvent *ev, bool flipY)
{
  // Make the parent window the current context
  QtAbstractOpenGLBox *parent = this->GetParentGLWidget();
  parent->makeCurrent();

  // Get the x,y coordinates of the event in actual pixels (retina)
  int xpix = (int) ev->x() * this->devicePixelRatio();
  int ypix = (int) ev->y() * this->devicePixelRatio();
  int hpix = (int) parent->height() * this->devicePixelRatio();
  int x = xpix;
  int y = (flipY) ? hpix - 1 - ypix : ypix;

  // Get the cell size and the number of cells
  Vector2ui sz = m_ParentModel->GetSize();
  DisplayLayoutModel *dlm = m_ParentModel->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui cells = dlm->GetSliceViewLayerTilingModel()->GetValue();
  int nrows = cells[0], ncols = cells[1];
  int icol, irow;

  // Determine which layout cell generated the event, unless we are dragging
  // in which case we want to use the last cell
  if(!this->isDragging())
    {
    // Which cell did the event fall in?
    icol = xpix / sz[0];
    irow = ypix / sz[1];

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
  // GLint viewport[] = { icol * sz[0], (nrows - 1 - irow) * sz[1], sz[0], sz[1] };
  GLint viewport[] = { 0, 0, ncols * sz[0], nrows * sz[1] };
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

  // Unproject to get the coordinate of the event
  Vector3d xProjection;
  gluUnProject(x, y, 0,
               modelMatrix,projMatrix,viewport,
               &xProjection[0], &xProjection[1], &xProjection[2]);

  // Get the within-cell coordinates
  xProjection[0] = fmod(xProjection[0], (double) m_ParentModel->GetSize()[0]);
  xProjection[1] = fmod(xProjection[1], (double) m_ParentModel->GetSize()[1]);
  return xProjection;
}
