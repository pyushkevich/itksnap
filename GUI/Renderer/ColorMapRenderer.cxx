#include "ColorMapRenderer.h"
#include "ColorMapModel.h"
#include "VTKRenderGeometry.h"
#include "SNAPOpenGL.h"
#include <vtkContextItem.h>
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkContextDevice2D.h>
#include <vtkImageData.h>
#include <vtkBrush.h>
#include <vtkPen.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkRenderWindow.h>
#include <vtkTransform2D.h>
#include <vtkPoints2D.h>
#include <vtkContextMouseEvent.h>
#include <iostream>

using namespace std;

class ColorMapContextItem : public vtkContextItem
{
public:
  vtkTypeMacro(ColorMapContextItem, vtkContextItem)
  static ColorMapContextItem *New();

  irisGetSetMacro(Model, ColorMapModel *)

  ColorMapContextItem()
  {
    // Allocate and initialize the texture for the checkerboard
    unsigned int boxw = 16;
    m_CheckerboardTexture = MakeTexture(boxw, boxw);
    unsigned char *data =
        reinterpret_cast<unsigned char *>(m_CheckerboardTexture->GetScalarPointer(0,0,0));

    for(unsigned int i = 0; i < boxw; i++)
      {
      for(unsigned int j = 0; j < boxw; j++)
        {
        unsigned char color = 0xff;
        if((i < boxw / 2 && j < boxw / 2) || (i >= boxw / 2 && j >= boxw / 2))
          color = 0xef;
        *data++ = color;
        *data++ = color;
        *data++ = color;
        *data++ = 255;
        }
      }

    m_CheckerboardTexture->Modified();

    // Allocate the texture for the colormap
    m_ColorMapMainTexture = MakeTexture(513, 17);
    m_ColorMapLeftTexture = MakeTexture(17, 17);
    m_ColorMapRightTexture = MakeTexture(17, 17);

    // Create the grid
    m_GridVertices = vtkNew<vtkPoints2D>();
    m_GridVertices->Allocate(16);

    m_GridVertices->InsertNextPoint(-0.1, 0.0); m_GridVertices->InsertNextPoint( 1.1, 0.0);
    m_GridVertices->InsertNextPoint(-0.1, 0.5); m_GridVertices->InsertNextPoint( 1.1, 0.5);
    m_GridVertices->InsertNextPoint(-0.1, 1.0); m_GridVertices->InsertNextPoint( 1.1, 1.0);

    // Vertical gridlines
    m_GridVertices->InsertNextPoint(0.0, -0.1); m_GridVertices->InsertNextPoint(0.0, 1.1);
    m_GridVertices->InsertNextPoint(0.25, -0.1); m_GridVertices->InsertNextPoint(0.25, 1.1);
    m_GridVertices->InsertNextPoint(0.5, -0.1); m_GridVertices->InsertNextPoint(0.5, 1.1);
    m_GridVertices->InsertNextPoint(0.75, -0.1); m_GridVertices->InsertNextPoint(0.75, 1.1);
    m_GridVertices->InsertNextPoint(1.0, -0.1); m_GridVertices->InsertNextPoint(1.0, 1.1);

    // Curve points
    m_CurveVertices = vtkNew<vtkPoints2D>();

    // Control point circle
    m_ControlOutline = VTKRenderGeometry::MakeUnitCircle(72);
    m_ControlDisk = VTKRenderGeometry::MakeUnitDisk(72);
    m_ControlLeftHalfDisk = VTKRenderGeometry::MakeWedge(36, 0.5 * vnl_math::pi, 1.5 * vnl_math::pi);
    m_ControlRightHalfDisk = VTKRenderGeometry::MakeWedge(36, -0.5 * vnl_math::pi, 0.5 * vnl_math::pi);
  }

  virtual vtkSmartPointer<vtkImageData> MakeTexture(unsigned int w, unsigned int h)
  {
    vtkSmartPointer<vtkImageData> texture = vtkSmartPointer<vtkImageData>::New();
    auto *info = texture->GetInformation();
    texture->SetExtent(0, w-1, 0, h-1, 0, 0);
    texture->SetPointDataActiveScalarInfo(info, VTK_UNSIGNED_CHAR, 4);
    texture->AllocateScalars(info);
    return texture;
  }

  virtual void SetTextureColumn(vtkImageData *texture, unsigned int i, double t)
  {
    ColorMap::RGBAType rgba = m_Model->GetColorMap()->MapIndexToRGBA(t);
    for(unsigned int j = 0; j <= 16; j++)
      {
      texture->SetScalarComponentFromDouble(i, j, 0, 0, rgba[0]);
      texture->SetScalarComponentFromDouble(i, j, 0, 1, rgba[1]);
      texture->SetScalarComponentFromDouble(i, j, 0, 2, rgba[2]);
      texture->SetScalarComponentFromDouble(i, j, 0, 3, 255 * j / 16.);
      }
  }

  virtual void UpdateColorMapTexture()
  {
    if(m_Model->GetColorMap())
      {
      for(unsigned int i = 0; i <= 16; i++)
        SetTextureColumn(m_ColorMapLeftTexture, i, 0.0);
      for(unsigned int i = 0; i <= 16; i++)
        SetTextureColumn(m_ColorMapRightTexture, i, 1.0);
      for(unsigned int i = 0; i <= 512; i++)
        SetTextureColumn(m_ColorMapMainTexture, i, i / 512.0);

      m_ColorMapMainTexture->Modified();
      m_ColorMapLeftTexture->Modified();
      m_ColorMapRightTexture->Modified();
      }
  }

  virtual void UpdateCurveGeometry()
  {
    if(m_Model->GetColorMap())
      {
      auto *cmap = m_Model->GetColorMap();
      ColorMap::RGBAType v0 = cmap->MapIndexToRGBA(-0.1);
      ColorMap::RGBAType v1 = cmap->MapIndexToRGBA(1.1);

      m_CurveVertices = vtkNew<vtkPoints2D>();
      m_CurveVertices->Allocate(2 * cmap->GetNumberOfCMPoints() + 2);

      m_CurveVertices->InsertNextPoint(-0.1, v0[3] / 255.0);

      for(size_t i = 0; i < cmap->GetNumberOfCMPoints(); i++)
        {
        ColorMap::CMPoint p = cmap->GetCMPoint(i);
        m_CurveVertices->InsertNextPoint(p.m_Index, p.m_RGBA[0][3] / 255.0);
        m_CurveVertices->InsertNextPoint(p.m_Index, p.m_RGBA[1][3] / 255.0);
        }

      m_CurveVertices->InsertNextPoint(1.1, v1[3] / 255.0);
      }
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    auto *cmap = m_Model->GetColorMap();
    vtkRecti rect = painter->GetDevice()->GetViewportRect();
    int w = rect.GetWidth(), h = rect.GetHeight();
    int vppr = m_Model->GetViewportReporter()->GetViewportPixelRatio();

    // Generate the texture for the background
    painter->GetBrush()->SetTexture(m_CheckerboardTexture);
    painter->GetBrush()->SetTextureProperties(vtkBrush::Linear | vtkBrush::Repeat);
    painter->GetBrush()->SetColorF(1.0, 1.0, 1.0);
    painter->DrawRect(0, 0, w, h);
    painter->GetBrush()->SetTexture(nullptr);

    if(cmap)
      {
      // Set the transform so that the window maps to range -0.1 to 1.1 in x/y
      vtkNew<vtkTransform2D> tran;
      tran->Scale(w / 1.2, h / 1.2);
      tran->Translate(0.1, 0.1);
      painter->PushMatrix();
      painter->SetTransform(tran);

      // Plot the central texture
      painter->GetBrush()->SetTextureProperties(vtkBrush::Linear | vtkBrush::Stretch);
      painter->GetBrush()->SetColorF(1.0, 1.0, 1.0);
      painter->GetPen()->SetLineType(vtkPen::NO_PEN);
      painter->GetBrush()->SetTexture(m_ColorMapMainTexture);
      painter->DrawRect(0, 0, 1.0, 1.0);

      // Plot left and right textures
      painter->GetBrush()->SetTexture(m_ColorMapLeftTexture);
      painter->DrawRect(-0.1, 0, 0.1, 1.0);
      painter->GetBrush()->SetTexture(m_ColorMapRightTexture);
      painter->DrawRect(1.0, 0, 0.1, 1.0);

      // Paint the grid
      painter->GetBrush()->SetTexture(nullptr);
      painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
      painter->GetPen()->SetColor(0x80, 0x80, 0x80);
      painter->GetPen()->SetOpacityF(0.75);
      painter->GetPen()->SetWidth(1.0 * vppr);
      painter->DrawLines(m_GridVertices);

      // Paint the curve
      painter->GetPen()->SetColor(0x0, 0x0, 0x0);
      painter->GetPen()->SetOpacityF(0.75);
      painter->GetPen()->SetWidth(3.0 * vppr);
      painter->DrawPoly(m_CurveVertices);

      // Paint the control points
      painter->GetPen()->SetWidth(1.5 * vppr);
      for(unsigned int i = 0; i < cmap->GetNumberOfCMPoints(); i++)
        {
        ColorMap::CMPoint p = cmap->GetCMPoint(i);
        bool is_split = (p.m_RGBA[0][3] != p.m_RGBA[1][3]);

        for(int side = 0; side < 2; side++)
          {
          auto &rgba = p.m_RGBA;
          bool select = m_Model->IsControlSelected(i, ColorMapLayerProperties::Side(side));
          if(select)
            painter->GetPen()->SetColor(0xff, 0x00, 0xff);
          else
            painter->GetPen()->SetColor(0x00, 0x00, 0x00);

          painter->PushMatrix();
          vtkNew<vtkTransform2D> tran;
          tran->Translate(p.m_Index, rgba[side][3] / 255.0);
          tran->Scale(6.0 / w, 6.0 / h);
          painter->AppendTransform(tran);

          if(is_split)
            {
            // Draw full control point, one color
            painter->GetBrush()->SetColor(rgba[side][0],rgba[side][1],rgba[side][2]);
            painter->DrawQuadStrip(m_ControlDisk);
            }
          else
            {
            painter->GetBrush()->SetColor(rgba[0][0],rgba[0][1],rgba[0][2]);
            painter->DrawQuadStrip(m_ControlLeftHalfDisk);
            painter->GetBrush()->SetColor(rgba[1][0],rgba[1][1],rgba[1][2]);
            painter->DrawQuadStrip(m_ControlRightHalfDisk);
            }

          painter->DrawPoly(m_ControlOutline);
          painter->PopMatrix();

          if(!is_split)
            break;
          }
        }

      painter->PopMatrix();
      }

    return true;
  }

  // We override this method to return true because all events in the parent scene should
  // be captured by this item
  virtual bool Hit(const vtkContextMouseEvent &) override
  {
    return true;
  }

  virtual Vector3d GetEventColorSpaceCoordinates(const vtkVector2f &pos)
  {
    int w = this->GetScene()->GetViewWidth();
    int h = this->GetScene()->GetViewHeight();
    return Vector3d(pos[0] * 1.2 / w - 0.1, pos[1] * 1.2 / h - 0.1, 0);
  }

  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse) override
  {
    // Transform into color map coordinates
    Vector3d x = GetEventColorSpaceCoordinates(mouse.GetScenePos());
    std::cout << "MouseButtonPressEvent " << x << std::endl;
    return m_Model->ProcessMousePressEvent(x);
  }

  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse) override
  {
    // Transform into color map coordinates
    Vector3d x = GetEventColorSpaceCoordinates(mouse.GetScenePos());
    return m_Model->ProcessMouseReleaseEvent(x);
  }

  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse) override
  {
    // Transform into color map coordinates
    if(mouse.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
      {
      Vector3d x = GetEventColorSpaceCoordinates(mouse.GetScenePos());
      return m_Model->ProcessMouseDragEvent(x);
      }

    else return false;
  }

protected:
  ColorMapModel *m_Model;

  vtkSmartPointer<vtkImageData> m_CheckerboardTexture;
  vtkSmartPointer<vtkImageData> m_ColorMapMainTexture, m_ColorMapLeftTexture, m_ColorMapRightTexture;
  vtkSmartPointer<vtkPoints2D> m_GridVertices, m_CurveVertices;
  vtkSmartPointer<vtkPoints2D> m_ControlOutline, m_ControlDisk;
  vtkSmartPointer<vtkPoints2D> m_ControlLeftHalfDisk, m_ControlRightHalfDisk;
};

vtkStandardNewMacro(ColorMapContextItem)

ColorMapRenderer::ColorMapRenderer()
{
  m_Width = 0;
  m_Height = 0;

  // Set up the item
  m_ContextItem = vtkNew<ColorMapContextItem>();
  this->m_ContextView->GetScene()->AddItem(m_ContextItem);
}

ColorMapRenderer::~ColorMapRenderer()
{

}

void
ColorMapRenderer
::SetModel(ColorMapModel *model)
{
  this->m_Model = model;
  m_ContextItem->SetModel(model);
  Rebroadcast(model, ModelUpdateEvent(), ModelUpdateEvent());
}

void ColorMapRenderer::OnUpdate()
{
  std::cout << "ColorMapRenderer::OnUpdate " << this << std::endl;
  if(m_Model)
    {
    m_Model->Update();
    m_ContextItem->UpdateColorMapTexture();
    m_ContextItem->UpdateCurveGeometry();
    m_ContextView->GetRenderWindow()->Render();
    }
}

void ColorMapRenderer::paintGL()
{
  std::cout << "ColorMapRenderer::paintGL" << std::endl;
  if(m_Model)
    Superclass::paintGL();
}

/*
void ColorMapRenderer::paintGL()
{
  // Clear the viewport
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);

  // Disable lighting
  glEnable(GL_TEXTURE_2D);

  // Generate the texture for the background
  const GLsizei boxw = 16;
  if(m_TextureId == 0xffffffff)
    {
    // Create a checkerboard
    unsigned char *boxarr = new unsigned char[boxw * boxw], *p = boxarr;
    for(GLsizei i = 0; i < boxw; i++)
      for(GLsizei j = 0; j < boxw; j++)
        if((i < boxw / 2 && j < boxw / 2) || (i > boxw / 2 && j > boxw / 2))
          *p++ = 0xef;
        else
          *p++ = 0xff;

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);

    // Properties for the texture
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // Turn off modulo-4 rounding in GL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, boxw, boxw, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, boxarr);
    delete[] boxarr;
    }

  // Draw the background pattern
  glBindTexture(GL_TEXTURE_2D, m_TextureId);

  glColor3ub(0xff,0xff,0xff);

  glBegin(GL_QUADS);
  double tx = m_Width * 1.0 / boxw;
  double ty = m_Height * 1.0 / boxw;
  glTexCoord2d(0.0, 0.0); glVertex2d(-0.1, -0.1);
  glTexCoord2d(tx, 0.0); glVertex2d(1.1, -0.1);
  glTexCoord2d(tx, ty); glVertex2d(1.1, 1.1);
  glTexCoord2d(0.0, ty); glVertex2d(-0.1, 1.1);
  glEnd();

  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);


  if(m_Model->GetLayer() != NULL && m_Model->GetColorMap() != NULL)
    {
    // Turn on alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Get the starting and ending color values
    ColorMap *cmap = m_Model->GetColorMap();
    ColorMap::RGBAType v0 = cmap->MapIndexToRGBA(-0.1);
    ColorMap::RGBAType v1 = cmap->MapIndexToRGBA(1.1);

    // Draw the actual texture
    glBegin(GL_QUADS);

    // Draw the color before 0.0
    glColor4ub(v0[0], v0[1], v0[2], 0x00); glVertex2d(-0.1,0.0);
    glColor4ub(v0[0], v0[1], v0[2], 0xff); glVertex2d(-0.1,1.0);

    // Render the colormap
    for(size_t i = 0; i < 512; i++)
      {
      double t = i / 511.0;
      ColorMap::RGBAType rgba = cmap->MapIndexToRGBA(t);

      glColor4ub(rgba[0], rgba[1], rgba[2], 0xff);
      glVertex2d(t, 1.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0x00);
      glVertex2d(t, 0.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0x00);
      glVertex2d(t, 0.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0xff);
      glVertex2d(t, 1.0);
      }

    // Draw the color after 1.0
    glColor4ub(v1[0], v1[1], v1[2], 0xff); glVertex2d(1.1,1.0);
    glColor4ub(v1[0], v1[1], v1[2], 0x00); glVertex2d(1.1,0.0);

    // Done drawing
    glEnd();

    // Draw the gridlines
    glBegin(GL_LINES);
    glColor4ub(0x80, 0x80, 0x80, 0xff);

    // Horizontal gridlines
    glVertex2d(-0.1, 0.0); glVertex2d( 1.1, 0.0);
    glVertex2d(-0.1, 0.5); glVertex2d( 1.1, 0.5);
    glVertex2d(-0.1, 1.0); glVertex2d( 1.1, 1.0);

    // Vertical gridlines
    glVertex2d(0.0, -0.1); glVertex2d(0.0, 1.1);
    glVertex2d(0.25, -0.1); glVertex2d(0.25, 1.1);
    glVertex2d(0.5, -0.1); glVertex2d(0.5, 1.1);
    glVertex2d(0.75, -0.1); glVertex2d(0.75, 1.1);
    glVertex2d(1.0, -0.1); glVertex2d(1.0, 1.1);

    glEnd();

    // Draw the Alpha curve
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(3.0);

    glBegin(GL_LINE_STRIP);
    glColor4ub(0x00,0x00,0x00,0xff);
    glVertex2d(-0.1, v0[3] / 255.0);

    for(size_t i = 0; i < cmap->GetNumberOfCMPoints(); i++)
      {
      ColorMap::CMPoint p = cmap->GetCMPoint(i);
      glVertex2d(p.m_Index, p.m_RGBA[0][3] / 255.0);
      glVertex2d(p.m_Index, p.m_RGBA[1][3] / 255.0);
      }

    glVertex2d(1.1, v1[3] / 255.0);
    glEnd();

    // Define select and non-select colors
    Vector3ui clrBorderSelect(0xff, 0x00, 0xff);
    Vector3ui clrBorderPlain(0x00, 0x00, 0x00);

    // Draw circles around the points
    glDisable(GL_LIGHTING);

    glLineWidth(1.0);
    for(size_t i = 0; i < cmap->GetNumberOfCMPoints(); i++)
      {
      ColorMap::CMPoint p = cmap->GetCMPoint(i);

      Vector3ui clrFill(p.m_RGBA[0][0],p.m_RGBA[0][1],p.m_RGBA[0][2]);
      bool select = m_Model->IsControlSelected(i, ColorMapLayerProperties::LEFT);
      gl_draw_circle_with_border(p.m_Index, p.m_RGBA[0][3] / 255.0, 5.0,
          1.2 / m_Width, 1.2 / m_Height,
          clrFill, 255,
          select ? clrBorderSelect : clrBorderPlain, 255,
          20);

      if(p.m_RGBA[0][3] != p.m_RGBA[1][3])
        {
        Vector3ui clrFill2(p.m_RGBA[1][0],p.m_RGBA[1][1],p.m_RGBA[1][2]);
        bool select = m_Model->IsControlSelected(i, ColorMapLayerProperties::RIGHT);
        gl_draw_circle_with_border(p.m_Index, p.m_RGBA[1][3] / 255.0, 5.0,
            1.2 / m_Width, 1.2 / m_Height,
            clrFill2, 255,
            select ? clrBorderSelect : clrBorderPlain, 255,
            20);
        }
      }
    }

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();

}

void ColorMapRenderer::resizeGL(int w, int h, int device_pixel_ratio)
{
  // Instead of using the w/h passed in here, which are in physical pixel units,
  // we will use 'logical' pixel units on Retina-type displays. This makes the
  // sizes of all circles and such consistent between Retina and normal displays

  if(!m_Model) return;

  Vector2ui vpl = m_Model->GetViewportReporter()->GetLogicalViewportSize();

  // Set up the basic projection with a small margin
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  irisOrtho2D(-0.1,1.1,-0.1,1.1);
  glViewport(0,0,vpl[0],vpl[1]);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Store the dimensions
  m_Width = vpl[0];
  m_Height = vpl[1];
}

*/
