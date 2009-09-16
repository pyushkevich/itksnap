/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMapWidget.h,v $
  Language:  C++
  Date:      $Date: 2009/09/16 20:03:13 $
  Version:   $Revision: 1.6 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __ColorMapWidget_h_
#define __ColorMapWidget_h_

#include "FLTKCanvas.h"
#include "ColorMap.h"

class LayerInspectorUILogic;
class ColorMapInteraction;

/**
 * \class ColorMapWidget
 * \brief An FLTK Box used to display a color map
 */
class ColorMapWidget : public FLTKCanvas
{
public:
  /** The standard FLTK window constructor */
  ColorMapWidget(int x,int y,int w,int h,const char *label=NULL);
  virtual ~ColorMapWidget();

  /** Set the current color map functor */
  void SetColorMap(const ColorMap &map)
    { m_ColorMap = map; redraw(); }

  /** Get the current color map functor */
  ColorMap &GetColorMap()
    { return m_ColorMap; }

  // Get/set the parent object
  irisGetMacro(Parent,LayerInspectorUILogic *);
  irisSetMacro(Parent,LayerInspectorUILogic *);

  /** The draw method */
  void draw();

  enum Side {LEFT, RIGHT, BOTH};

  irisGetMacro(SelectedSide, Side);
  irisGetMacro(SelectedCMPoint, int);
  void SetSelectedCMPoint(int pt);
  void SetSelectedSide(Side side);

  /** Sets selected point and side, returns true if changed */
  bool SetSelection(int pt, Side side);

private:
  ColorMap m_ColorMap;

  unsigned int m_TextureId;

  int m_SelectedCMPoint;
  Side m_SelectedSide;

  void gl_draw_circle_with_border(double x, double y, double r, bool select);

  /** Parent object */
  LayerInspectorUILogic *m_Parent;

  ColorMapInteraction *m_Interactor;

  friend class ColorMapInteraction;
};

class ColorMapInteraction : public InteractionMode
{
public:
  ColorMapInteraction(ColorMapWidget *parent) : InteractionMode(parent)
    { this->m_Parent = parent; }

  int OnMousePress(const FLTKEvent &e);
  int OnMouseDrag(const FLTKEvent &e, const FLTKEvent &pe);
  int OnMouseRelease(const FLTKEvent &e, const FLTKEvent &pe);

private:
  ColorMapWidget *m_Parent;
};

#endif
