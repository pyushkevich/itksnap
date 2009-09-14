/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/09/14 19:04:52 $
  Version:   $Revision: 1.10 $
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

#include "LayerInspectorUILogic.h"
#include "IntensityCurveVTK.h"
#include "UserInterfaceLogic.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GreyImageWrapper.h"

LayerInspectorUILogic
::LayerInspectorUILogic(UserInterfaceLogic *parent)
{
  m_Parent = parent;
  m_Driver = m_Parent->GetDriver();
  m_MainWrapper = NULL;
  m_OverlayWrappers = NULL;
  m_SelectedWrapper = NULL;
  m_GreyWrapper = NULL;
  m_Curve = NULL;
}

void
LayerInspectorUILogic
::Initialize()
{
  // Color map
  m_BoxColorMap->SetParent(this);
  PopulateColorMapPresets();

  // Intensity curve
  m_BoxCurve->SetParent(this);

  // Opacity
  m_InOverallOpacity->maximum(255);
  m_InOverallOpacity->minimum(0);
  m_InOverallOpacity->step(1);
}

void
LayerInspectorUILogic
::SetImageWrappers()
{
  // Connect the image wrappers
  m_MainWrapper = m_Driver->GetCurrentImageData()->GetMain();
  m_OverlayWrappers = m_Driver->GetCurrentImageData()->GetOverlays();

  // Build layer browser entries
  m_BrsLayers->clear();
  m_BrsLayers->add("MAIN");
  for (size_t i = 0; i < m_OverlayWrappers->size(); ++i)
    {
    m_BrsLayers->add("OVERLAY");
    }
  // Select the last loaded image by default
  m_BrsLayers->select(1 + m_OverlayWrappers->size());
  OnLayerSelectionUpdate();
}

void
LayerInspectorUILogic
::DisplayWindow()
{
  if (m_LayerUITabs->value() == m_ImageContrastTab)
    {
    DisplayImageContrastTab(); 
    }
  else if (m_LayerUITabs->value() == m_ColorMapTab)
    {
    DisplayColorMapTab();
    }
  else
    {
    DisplayImageInfoTab();
    }
}

void
LayerInspectorUILogic
::DisplayImageContrastTab()
{
  m_BoxColorMap->hide();
  m_WinLayerUI->show();
  m_LayerUITabs->value(m_ImageContrastTab);
  if (m_ImageContrastTab->active())
    {
    UpdateWindowAndLevel();
    m_BoxCurve->redraw();
    m_BoxCurve->show();
    }
}

void
LayerInspectorUILogic
::DisplayColorMapTab()
{
  m_BoxCurve->hide();
  m_WinLayerUI->show();
  m_LayerUITabs->value(m_ColorMapTab);
  if (m_ColorMapTab->active())
    {
    // Determine the currently used color map
    ColorMap cm = m_GreyWrapper->GetColorMap();
    // Update the GUI
    m_InColorMapPreset->value(cm.GetSystemPreset());
    m_BoxColorMap->SetColorMap(cm);
    m_BoxColorMap->redraw();
    m_BoxColorMap->show();
    }
}

void
LayerInspectorUILogic
::DisplayImageInfoTab()
{
  m_BoxCurve->hide();
  m_BoxColorMap->hide();
  m_WinLayerUI->show();
  m_LayerUITabs->value(m_ImageInfoTab);
}

void
LayerInspectorUILogic
::RedrawWindow()
{
  // Intensity curve
  if (m_LayerUITabs->value() == m_ImageContrastTab && m_ImageContrastTab->active())
    {
    UpdateWindowAndLevel();
    m_BoxCurve->redraw();
    m_BoxCurve->show();
    }
  // Color map
  else if (m_LayerUITabs->value() == m_ColorMapTab && m_ColorMapTab->active())
    {
    // Determine the currently used color map
    ColorMap cm = m_GreyWrapper->GetColorMap();
    // Update the GUI
    m_InColorMapPreset->value(cm.GetSystemPreset());
    m_BoxColorMap->SetColorMap(cm);
    m_BoxColorMap->redraw();
    m_BoxColorMap->show();
    }
  // Image info
  else if (m_LayerUITabs->value() == m_ImageInfoTab)
    {
    }
}

void
LayerInspectorUILogic
::Hide()
{
  m_WinLayerUI->hide();
}

bool
LayerInspectorUILogic
::Shown()
{
  return m_WinLayerUI->shown();
}

// Callbacks for the main pane
void 
LayerInspectorUILogic
::OnLayerSelectionUpdate()
{
  // If user click the empty space in the browser window
  if (m_BrsLayers->value() == 0)
    return;

  // Determine the corresponding image wrapper
  if (m_BrsLayers->value() == 1)
    {
    m_SelectedWrapper = m_MainWrapper;
    m_InOverallOpacity->deactivate();
    }
  else
    {
    WrapperIterator it = m_OverlayWrappers->begin();
    for (int i = 2; i < m_BrsLayers->value(); ++i)
      ++it;
    assert(it != m_OverlayWrappers->end());
    m_SelectedWrapper = *it;
    m_InOverallOpacity->activate();
    }

  // Overall Opacity
  m_InOverallOpacity->value(m_SelectedWrapper->GetAlpha());

  // Hook it up with the GUI
  m_GreyWrapper = dynamic_cast<GreyImageWrapper *>(m_SelectedWrapper);
  if (m_GreyWrapper)
    {
    m_ImageContrastTab->activate();
    m_ColorMapTab->activate();
    // associate with intensity curve UI
    m_Curve = m_GreyWrapper->GetIntensityMapFunction();
    if (m_Curve->GetControlPointCount() == 3)
      m_BtnCurveLessControlPoint->deactivate();
    m_BoxCurve->SetCurve(m_Curve);
    m_BoxCurve->SetHistogramBinSize(1);
    m_BoxCurve->ComputeHistogram(m_GreyWrapper, 4);

    m_InHistogramMaxLevel->value(m_BoxCurve->GetHistogramMaxLevel() * 100.f);
    m_InHistogramBinSize->value(m_BoxCurve->GetHistogramBinSize());
    m_ChkHistogramLog->value(m_BoxCurve->IsHistogramLog());
    // associate with color map UI (if needed)
    // associate with image info: greyscale specific
    m_OutImageInfoRange[0]->value(m_GreyWrapper->GetImageMinNative());
    m_OutImageInfoRange[1]->value(m_GreyWrapper->GetImageMaxNative());
    }
  else
    {
    m_ImageContrastTab->deactivate();
    m_BoxCurve->hide();
    m_ColorMapTab->deactivate();
    m_BoxColorMap->hide();
    // associate with image info: RGB specific
    m_OutImageInfoRange[0]->value(0);
    m_OutImageInfoRange[1]->value(0);
    }

  // associate with image info: common
  for (size_t d = 0; d < 3; ++d)
    {
    m_OutImageInfoDimensions[d]->value(m_SelectedWrapper->GetSize()[d]);
    m_OutImageInfoOrigin[d]->value(m_SelectedWrapper->GetImageBase()->GetOrigin()[d]);
    m_OutImageInfoSpacing[d]->value(m_SelectedWrapper->GetImageBase()->GetSpacing()[d]);

    m_OutImageInfoCursorPosition[d]->precision(2);
    m_OutImageInfoCursorNIFTIPosition[d]->precision(2);

    m_InImageInfoCursorIndex[d]->maximum(m_SelectedWrapper->GetSize()[d] - 1);
    m_InImageInfoCursorIndex[d]->minimum(0);
    m_InImageInfoCursorIndex[d]->step(1);
    }

  // get the RAI code
  ImageCoordinateGeometry::DirectionMatrix dmat =
    m_Driver->GetCurrentImageData()->GetImageGeometry().GetImageDirectionCosineMatrix();
  string raicode =
    ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(dmat);
  string raitext;
  if (ImageCoordinateGeometry::IsDirectionMatrixOblique(dmat))
    raitext = string("Oblique (closest to ") + raicode + string(")");
  else
    raitext = raicode;
  m_OutImageInfoOriginRAICode->value(raitext.c_str());

  // update GUI
  if (Shown())
    RedrawWindow();
}

void 
LayerInspectorUILogic
::OnOverallOpacityUpdate()
{
  m_SelectedWrapper->SetAlpha(m_InOverallOpacity->value());
}

void 
LayerInspectorUILogic
::OnCloseAction()
{
  m_WinLayerUI->hide();
}

// Callbacks for the contrast adjustment page
void 
LayerInspectorUILogic
::OnCurveReset()
{
  // Reinitialize the curve
  m_Curve->Initialize();

  // Fire the reset event
  OnCurveChange();
}

void
LayerInspectorUILogic
::UpdateWindowAndLevel()
{
  // Need a curve and a wrapper
  assert(m_Curve && m_GreyWrapper);

  // This is the range of the curve in unit coordinates (0 to 1)
  float t0,x0,t1,x1;

  // Get the starting and ending control points
  m_Curve->GetControlPoint(0,t0,x0);
  m_Curve->GetControlPoint(m_Curve->GetControlPointCount()-1,t1,x1);

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  double iAbsMin = m_GreyWrapper->GetImageMinNative();
  double iAbsMax = m_GreyWrapper->GetImageMaxNative();
  double iAbsSpan = (iAbsMax - iAbsMin);

  // The the curve intensity range
  double iMin = iAbsMin + iAbsSpan * t0; 
  double iMax = iAbsMin + iAbsSpan * t1;

  // Compute the level and window in intensity units
  double level = iMin;
  double window = iMax - iMin;

  // Compute and constrain the level 
  m_InLevel->value(level);
  m_InLevel->minimum(iAbsMin);
  m_InLevel->maximum(iAbsMax - window);

  // Compute and constrain the window
  m_InWindow->value(window);
  m_InWindow->minimum(0.01);
  m_InWindow->maximum(iAbsMax - level);
}

void
LayerInspectorUILogic
::OnCurveChange()
{
  m_BoxColorMap->hide();
  // Update the values of the window and level
  UpdateWindowAndLevel();

  // Redraw the window
  m_BoxCurve->redraw();
  m_BoxCurve->show();

  // Update the image wrapper
  m_GreyWrapper->UpdateIntensityMapFunction();
}

void 
LayerInspectorUILogic
::OnAutoFitWindow()
{
  // Get the histogram
  const std::vector<unsigned int> &hist = m_BoxCurve->GetHistogram();

  // Integrate the histogram until reaching 0.1%
  GreyType imin = m_GreyWrapper->GetImageMin();
  GreyType ilow = imin;
  size_t accum = 0;
  size_t accum_goal = m_GreyWrapper->GetNumberOfVoxels() / 1000;
  for(size_t i = 0; i < hist.size(); i++)
    {
    if(accum + hist[i] < accum_goal)
      {
      accum += hist[i];
      ilow += m_BoxCurve->GetHistogramBinSize();
      }
    else break;
    }

  // Same, but from above
  GreyType imax = m_GreyWrapper->GetImageMax();
  GreyType ihigh = imax;
  accum = 0;
  for(size_t i = hist.size() - 1; i >= 0; i--)
    {
    if(accum + hist[i] < accum_goal)
      {
      accum += hist[i];
      ihigh -= m_BoxCurve->GetHistogramBinSize();
      }
    else break;
    }

  // If for some reason the window is off, we set everything to max/min
  if(ilow >= ihigh)
    { ilow = imin; ihigh = imax; }

  // Compute and constrain the window
  GreyTypeToNativeFunctor native = m_GreyWrapper->GetNativeMapping();
  double ilowNative = native(ilow);
  double ihighNative = native(ihigh);
  double imaxNative = native(imax);
  double iwin = ihighNative - ilowNative;

  m_InWindow->maximum(imaxNative - ilowNative);
  m_InWindow->value(iwin);

  m_InLevel->maximum(imaxNative - iwin);
  m_InLevel->value(ilowNative);  

  OnWindowLevelChange();
}


void 
LayerInspectorUILogic
::OnWindowLevelChange()
{
  // Assure that input and output outside of the image range
  // is handled gracefully
  m_InLevel->value(m_InLevel->clamp(m_InLevel->value()));
  m_InWindow->value(m_InWindow->clamp(m_InWindow->value()));

  // Get 'absolute' image intensity range, i.e., the largest and smallest
  // intensity in the whole image
  double iAbsMin = m_GreyWrapper->GetImageMinNative();
  double iAbsMax = m_GreyWrapper->GetImageMaxNative();

  // Get the new values of min and max
  double iMin = m_InLevel->value();
  double iMax = iMin + m_InWindow->value();

  // Min better be less than max
  assert(iMin < iMax);

  // Compute the unit coordinate values that correspond to min and max
  float factor = 1.0f / (iAbsMax - iAbsMin);
  float t0 = factor * (iMin - iAbsMin);
  float t1 = factor * (iMax - iAbsMin);

  // Update the curve boundary
  m_Curve->ScaleControlPointsToWindow(t0,t1);

  // Fire the reset event
  OnCurveChange();
}

void 
LayerInspectorUILogic
::OnUpdateHistogram()
{
  // Recompute the histogram and redisplay
  m_BoxCurve->SetHistogramBinSize((size_t) m_InHistogramBinSize->value());
  m_BoxCurve->SetHistogramMaxLevel(m_InHistogramMaxLevel->value() / 100.0f);
  m_BoxCurve->SetHistogramLog(m_ChkHistogramLog->value() ? true : false);
  m_BoxCurve->ComputeHistogram(m_GreyWrapper, 1);
  m_BoxCurve->redraw();

  // The histogram controls may have changed. Update them
  m_InHistogramMaxLevel->value(m_BoxCurve->GetHistogramMaxLevel() * 100.0);
  m_InHistogramBinSize->value(m_BoxCurve->GetHistogramBinSize());
}

void 
LayerInspectorUILogic
::OnControlPointMoreAction()
{
  m_Curve->Initialize(m_Curve->GetControlPointCount() + 1);
  if (m_Curve->GetControlPointCount() > 3)
    m_BtnCurveLessControlPoint->activate();
  OnWindowLevelChange();
}

void 
LayerInspectorUILogic
::OnControlPointLessAction()
{
  if (m_Curve->GetControlPointCount() > 3)
    {
    m_Curve->Initialize(m_Curve->GetControlPointCount() - 1);
    OnWindowLevelChange();
    }
  if (m_Curve->GetControlPointCount() == 3)
    m_BtnCurveLessControlPoint->deactivate();
}

void
LayerInspectorUILogic
::OnControlPointChange()
{
  float f = m_InControlX->value();
  float x = m_InControlY->value();
  int cp = m_BoxCurve->GetInteractor()->GetMovingControlPoint();
  m_Curve->UpdateControlPoint(cp, f, x);
  OnCurveChange();
}

void 
LayerInspectorUILogic
::OnControlPointUpdate()
{
  int cp = m_BoxCurve->GetInteractor()->GetMovingControlPoint();
  float f, x;
  m_Curve->GetControlPoint(cp, f, x);
  m_InControlX->value(f);
  m_InControlY->value(x);
}

// Callbacks for the color map page
LayerInspectorUILogic::PresetInfo
LayerInspectorUILogic::m_PresetInfo[] = {
  {"Grayscale", 0x00ff0000},
  {"Black to red", 0x00ff0000},
  {"Black to green", 0xff000000},
  {"Black to blue", 0xff000000},
  {"Hot", 0x00000000},
  {"Cool", 0x00000000},
  {"Spring", 0x00000000},
  {"Summer", 0x00000000},
  {"Autumn", 0x00000000},
  {"Winter", 0x00000000},
  {"Copper", 0x00000000},
  {"HSV", 0x00000000},
  {"Jet", 0x00000000},
  {"OverUnder", 0x00000000}};

void
LayerInspectorUILogic
::OnColorMapChange()
{
  m_GreyWrapper->SetColorMap(m_BoxColorMap->GetColorMap());
  m_GreyWrapper->UpdateIntensityMapFunction();
}

void 
LayerInspectorUILogic
::PopulateColorMapPresets()
{
  m_InColorMapPreset->clear();
  for(int i = 0; i < ColorMap::COLORMAP_SIZE; i++)
    m_InColorMapPreset->add(m_PresetInfo[i].name.c_str());
}

void 
LayerInspectorUILogic
::OnColorMapPresetUpdate()
{
  m_BoxCurve->hide();
  // Apply the current preset
  int sel = m_InColorMapPreset->value();
  ColorMap::SystemPreset preset = 
    (ColorMap::SystemPreset) (ColorMap::COLORMAP_GREY + sel);

  // Update the color map
  ColorMap cm;
  cm.SetToSystemPreset(preset);
  m_BoxColorMap->SetColorMap(cm);
  m_BoxColorMap->redraw();
  m_BoxColorMap->show();

  // Update the image
  m_GreyWrapper->SetColorMap(cm);
  m_GreyWrapper->UpdateIntensityMapFunction();
}

void 
LayerInspectorUILogic
::OnColorMapAddPresetAction()
{

}

void 
LayerInspectorUILogic
::OnColorMapDeletePresetAction()
{

}

void 
LayerInspectorUILogic
::OnColorMapIndexUpdate()
{

}

void 
LayerInspectorUILogic
::OnColorMapSideUpdate()
{

}

void 
LayerInspectorUILogic
::OnColorMapPointDelete()
{

}

void 
LayerInspectorUILogic
::OnColorMapRGBAUpdate()
{

}

// Callbacks for the image info page
void
LayerInspectorUILogic
::UpdateImageProbe()
{
  // Code common to SNAP and IRIS
  Vector3ui crosshairs = m_Driver->GetCursorPosition();
  Vector3d xPosition = m_SelectedWrapper->TransformVoxelIndexToPosition(crosshairs);
  Vector3d xNIFTI = m_SelectedWrapper->TransformVoxelIndexToNIFTICoordinates(crosshairs);
  for (size_t d = 0; d < 3; ++d)
    {
    m_InImageInfoCursorIndex[d]->value(crosshairs[d]);
    m_OutImageInfoCursorPosition[d]->value(xPosition[d]);
    m_OutImageInfoCursorNIFTIPosition[d]->value(xNIFTI[d]);
    }
}

void 
LayerInspectorUILogic
::OnImageInformationVoxelCoordinatesUpdate()
{
  // Read the cursor values
  Vector3ui cpos;
  for (size_t d = 0; d < 3; ++d)
    {
    cpos[d] = (unsigned int) m_InImageInfoCursorIndex[d]->clamp(
        m_InImageInfoCursorIndex[d]->round(
          m_InImageInfoCursorIndex[d]->value()));
    }
  m_Driver->SetCursorPosition(cpos);
  m_Parent->OnCrosshairPositionUpdate();
  m_Parent->RedrawWindows();
}

