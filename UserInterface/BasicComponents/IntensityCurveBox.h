/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveBox.h,v $
  Language:  C++
  Date:      $Date: 2009/09/12 23:04:29 $
  Version:   $Revision: 1.2 $
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
#ifndef __IntensityCurveBox_h_
#define __IntensityCurveBox_h_

#include <FL/Fl_Gl_Window.H>

#include <SNAPCommonUI.h>
#include <IntensityCurveInterface.h>
#include <FLTKCanvas.h>
#include <InteractionMode.h>

class LayerInspectorUILogic;
class GreyImageWrapper;
class IntensityCurveInteraction;

/**
 * \class IntensityCurveBox
 * \brief An FLTK Box (Gl_Window) used to paint intensity mapping curves.
 */
class IntensityCurveBox : public FLTKCanvas {
public:
  IntensityCurveBox(int x,int y,int w,int h,const char *label);
  virtual ~IntensityCurveBox();

  // Get the intensity curve interactor
  irisGetMacro(Interactor, IntensityCurveInteraction *);

  /**
   * Handle displaying the curve
   */
  void draw();

  /** Compute the histogram given an image wrapper */
  void ComputeHistogram(GreyImageWrapper *source,
    unsigned int iMinPixelsPerBin = 1);

  // Get/set the intensity curve
  irisGetMacro(Curve,IntensityCurveInterface *);
  irisSetMacro(Curve,IntensityCurveInterface *);

  // Get/set the parent object
  irisGetMacro(Parent,LayerInspectorUILogic *);
  irisSetMacro(Parent,LayerInspectorUILogic *);

  // Get the histogram itself
  const std::vector<unsigned int> &GetHistogram() const
    { return m_Histogram; }

  // Get/set the histogram properties
  irisGetMacro(HistogramBinSize, unsigned int);
  irisSetMacro(HistogramBinSize, unsigned int);
  irisGetMacro(HistogramMaxLevel, float);
  irisSetMacro(HistogramMaxLevel, float);
  irisIsMacro(HistogramLog);
  irisSetMacro(HistogramLog, bool);

  /**
   * The resolution of the curve displayed on the screen
   * TODO: Control over curve resolution
   */
  static unsigned int CURVE_RESOLUTION;

private:

  /**
   * Check if a control point is close to another point (i.e. mouse position)
   */
  int GetControlPointInVicinity(float x, float y, int pixelRadius);

  /** The intensity mapping curve */
  IntensityCurveInterface *m_Curve;

  /** Parent object */
  LayerInspectorUILogic *m_Parent;

  /** Histogram of the image */
  std::vector<unsigned int> m_Histogram;   

  /** Max frequency in the histogram */
  unsigned int m_HistogramMax;

  /** Max level in the histograms: bins above this level are truncated */
  float m_HistogramMaxLevel;

  /** Size of the bin, in intensities */
  unsigned int m_HistogramBinSize;

  /** Flag, whether log of the frequencies is used */
  bool m_HistogramLog;

  IntensityCurveInteraction* m_Interactor;

  // Allow access to private data
  friend class IntensityCurveInteraction;

};

/**
 * Interaction handler for control point manipulation
 */
class IntensityCurveInteraction : public InteractionMode {
public:
  IntensityCurveInteraction(IntensityCurveBox *parent);

  int OnMousePress(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent);
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
  int OnMouseEnter(const FLTKEvent &event);
  int OnMouseLeave(const FLTKEvent &event);
  int OnMouseMotion(const FLTKEvent &event);
  int GetMovingControlPoint() const;

private:
  // Pointer to the parent canvas
  IntensityCurveBox *m_Parent;

  // Set cursor depending on selected point
  void SetCursor(int iControlPoint);

  // Update a control point to reflect mouse motion to p
  bool UpdateControl(const Vector3f &p);

  // Control point being currently edited
  int m_MovingControlPoint;

};

#endif // __IntensityCurveBox_h_

