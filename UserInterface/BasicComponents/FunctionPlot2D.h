/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: FunctionPlot2D.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __FunctionPlot2D_h_
#define __FunctionPlot2D_h_

#include "SNAPCommonUI.h"
#include <vector>

/**
 * Settings for plotting 2D functions.  Based on Mathematica options
 * to the Plot command (a small subset, of course).
 */
class FunctionPlot2DSettings
{
public:
    virtual ~FunctionPlot2DSettings() {} ;

  irisGetMacro(PlotRangeMin,Vector2f);
  irisSetMacro(PlotRangeMin,Vector2f);

  irisGetMacro(PlotRangeMax,Vector2f);
  irisSetMacro(PlotRangeMax,Vector2f);

  irisGetMacro(AxesPosition,Vector2f);
  irisSetMacro(AxesPosition,Vector2f);

  irisGetMacro(MajorTickSpacing,Vector2f);
  irisSetMacro(MajorTickSpacing,Vector2f);

  irisGetMacro(MinorTickSpacing,Vector2f);
  irisSetMacro(MinorTickSpacing,Vector2f);

  irisGetMacro(ShowAxes,bool);
  irisSetMacro(ShowAxes,bool);

  irisGetMacro(ShowFrame,bool);
  irisSetMacro(ShowFrame,bool);

  irisGetMacro(ShowMajorTicks,bool);
  irisSetMacro(ShowMajorTicks,bool);

  irisGetMacro(ShowMinorTicks,bool);
  irisSetMacro(ShowMinorTicks,bool);

  irisGetMacro(AxesColor,Vector3f);
  irisSetMacro(AxesColor,Vector3f);

  irisGetMacro(FrameColor,Vector3f);
  irisSetMacro(FrameColor,Vector3f);
  
  irisGetMacro(PlotColor,Vector3f);
  irisSetMacro(PlotColor,Vector3f);

  irisGetMacro(BackgroundColor,Vector3f);
  irisSetMacro(BackgroundColor,Vector3f);

  static FunctionPlot2DSettings GetDefaultSettings();

private:
  Vector2f m_PlotRangeMin;
  Vector2f m_PlotRangeMax;
  Vector2f m_AxesPosition;
  Vector2f m_MajorTickSpacing;
  Vector2f m_MinorTickSpacing;

  bool m_ShowAxes;
  bool m_ShowFrame;
  bool m_ShowMajorTicks;
  bool m_ShowMinorTicks;
  
  Vector3f m_AxesColor;
  Vector3f m_FrameColor;
  Vector3f m_PlotColor;
  Vector3f m_BackgroundColor;
};

/**
 * \class FunctionPlot2D
 * \brief A UI component for plotting 2D graphs using GL functions.
 */
class FunctionPlot2D
{
public:
  /** Get a reference to the settings in this object */
  FunctionPlot2DSettings &GetSettings() 
  {
    return m_Settings;
  }

  /** Set the data to be plotted */
  void SetDataPoints(float *xPoints,float *yPoints,unsigned int nPoints);

  /** 
   * Draw the function in the standard OpenGL context.  The method will place
   * the plot inside the region ((0,0),(1,1)).  
   */
  void Draw();

  /** Constructor, takes on default plot settings */
  FunctionPlot2D();
  
private:
  typedef std::vector<Vector2f> PointVector;

  FunctionPlot2DSettings m_Settings;  
  PointVector m_Points;
};

#endif
