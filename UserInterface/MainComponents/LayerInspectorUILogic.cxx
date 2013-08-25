/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 15:06:07 $
  Version:   $Revision: 1.27 $
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
#include "IRISException.h"
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
  GreyImageWrapper *wrapper = dynamic_cast<GreyImageWrapper *>(m_MainWrapper);
  if (wrapper)
    m_BrsLayers->add("Main Image (Greyscale)");
  else
    m_BrsLayers->add("Main Image (RGB)");

  WrapperIterator it = m_OverlayWrappers->begin();
  while (it != m_OverlayWrappers->end())
    {
    GreyImageWrapper *wrapper = dynamic_cast<GreyImageWrapper *>(*it);
    if (wrapper)
      m_BrsLayers->add("Overlay (Greyscale)");
    else
      m_BrsLayers->add("Overlay (RGB)");
    ++it;
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
  else if (m_LayerUITabs->value() == m_ImageInfoTab)
    {
    DisplayImageInfoTab();
    }
  else if (m_LayerUITabs->value() == m_ImageAdvancedInfoTab)
    {
    DisplayImageAdvancedInfoTab();
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
    this->OnColorMapSelectedPointUpdate();
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
  UpdateImageProbe();
}


void
LayerInspectorUILogic
::DisplayImageAdvancedInfoTab()
{
  m_BoxCurve->hide();
  m_BoxColorMap->hide();
  m_WinLayerUI->show();
  m_LayerUITabs->value(m_ImageAdvancedInfoTab);
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
    UpdateImageProbe();
    }
  else if (m_LayerUITabs->value() == m_ImageAdvancedInfoTab)
    {
    }
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
  for (unsigned int d = 0; d < 3; ++d)
    {
    m_OutImageInfoDimensions[d]->value(m_SelectedWrapper->GetSize()[d]);
    m_OutImageInfoOrigin[d]->value(m_SelectedWrapper->GetImageBase()->GetOrigin()[d]);
    m_OutImageInfoOrigin[d]->maximum(m_OutImageInfoOrigin[d]->value());
    m_OutImageInfoOrigin[d]->minimum(m_OutImageInfoOrigin[d]->value());
    m_OutImageInfoSpacing[d]->value(m_SelectedWrapper->GetImageBase()->GetSpacing()[d]);
    m_OutImageInfoSpacing[d]->maximum(m_OutImageInfoSpacing[d]->value());
    m_OutImageInfoSpacing[d]->minimum(m_OutImageInfoSpacing[d]->value());
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

  // Uodate the information table
  m_TableMetaData->SetInputImage(m_SelectedWrapper->GetImageBase());
  m_TableMetaData->SetColumnWidth(370);

  // update GUI
  if (Shown())
    RedrawWindow();
}

void 
LayerInspectorUILogic
::OnOverallOpacityUpdate()
{
  m_SelectedWrapper->SetAlpha((unsigned char) m_InOverallOpacity->value());
  m_Parent->RedrawWindows();
}

void
LayerInspectorUILogic
::AdjustOverlayOpacity(double delta)
{
  // If the selected wrapper is an overlay, adjust it's opacity
  if(m_SelectedWrapper != m_MainWrapper)
    {
    double alpha = (double) m_SelectedWrapper->GetAlpha() + delta;
    if(alpha < 0) alpha = 0;
    if(alpha > 255) alpha = 255;
    m_SelectedWrapper->SetAlpha((unsigned char) alpha);
    m_InOverallOpacity->value(alpha);
    }

  // Otherwise, adjust all the overlays
  else
    {
    for(WrapperList::iterator it = m_OverlayWrappers->begin();
      it != m_OverlayWrappers->end(); it++)
      {
      double alpha = (double) (*it)->GetAlpha() + delta;
      if(alpha < 0) alpha = 0;
      if(alpha > 255) alpha = 255;
      (*it)->SetAlpha((unsigned char) alpha);
      }
    }

  m_Parent->RedrawWindows();
}

void
LayerInspectorUILogic
::ToggleOverlayVisibility()
{
  // If the selected wrapper is an overlay, adjust it's opacity
  if(m_SelectedWrapper != m_MainWrapper)
    {
    // Get current alpha value
    m_SelectedWrapper->ToggleVisibility();
    m_InOverallOpacity->value(m_SelectedWrapper->GetAlpha());
    }

  // Otherwise, adjust all the overlays
  else
    {
    for(WrapperList::iterator it = m_OverlayWrappers->begin();
      it != m_OverlayWrappers->end(); it++)
      {
      (*it)->ToggleVisibility();
      }
    }  

  m_Parent->RedrawWindows();
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

  // Update the controlX/controlY
  OnControlPointUpdate();

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

  // The step is set dynamically, so that there are on the order of 
  // 1000 steps to cover the whole intensity range. That should give
  // a good tradeoff between precision and control. 
  double step = pow(10, floor(0.5 + log10(iAbsSpan) - 3)); 
  m_InWindow->step(step);
  m_InWindow->step(step);
  m_InControlX->step(step);

  // Compute and constrain the level 
  m_InLevel->value(level);
  m_InLevel->minimum(iAbsMin);
  m_InLevel->maximum(iAbsMax - window);

  // Compute and constrain the window
  m_InWindow->value(window);
  m_InWindow->minimum(step);
  m_InWindow->maximum(iAbsMax - level);

  // In addition to window and level, we set up the valuators for
  // the current control point
  int cp = m_BoxCurve->GetInteractor()->GetMovingControlPoint();
  if(cp >= 0)
    {
    if(cp == 0)
      {
      m_InControlX->minimum(iAbsMin);
      }
    else
      {
      m_Curve->GetControlPoint(cp - 1, t0, x0);
      m_InControlX->minimum(iAbsMin + iAbsSpan * t0 + step);
      }

    if(cp == (int)(m_Curve->GetControlPointCount() - 1))
      {
      m_InControlX->maximum(iAbsMax);
      }
    else
      {
      m_Curve->GetControlPoint(cp + 1, t1, x1);
      m_InControlX->maximum(iAbsMin + iAbsSpan * t1 - step);
      }

    if(cp == 0 || cp == (int)(m_Curve->GetControlPointCount() - 1))
      {
      m_InControlY->deactivate();
      }
    else
      {
      m_InControlY->activate();
      m_InControlY->minimum(x0 + 0.01);
      m_InControlY->maximum(x1 + 0.01);
      m_InControlY->step(0.01);
      }    

    // Set the actual value
    float t, x;
    m_Curve->GetControlPoint(cp, t, x);
    m_InControlX->activate();
    m_InControlX->value(iAbsMin + iAbsSpan * t);
    m_InControlY->value(x);
    }
  else
    {
    m_InControlX->deactivate();
    m_InControlY->deactivate();
    }
}

void
LayerInspectorUILogic
::OnCurveChange()
{
  if(m_WinLayerUI->shown())
    {
    m_BoxColorMap->hide();
    }

  // Update the values of the window and level
  UpdateWindowAndLevel();

  // Redraw the window
  if(m_WinLayerUI->shown())
    {
    m_BoxCurve->redraw();
    m_BoxCurve->show();
    }

  // Update the image wrapper
  m_GreyWrapper->UpdateIntensityMapFunction();

  // Redraw parent
  m_Parent->RedrawWindows();
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
  for(int i = hist.size() - 1; i >= 0; i--)
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
  // m_InLevel->value(m_InLevel->clamp(m_InLevel->value()));
  // m_InWindow->value(m_InWindow->clamp(m_InWindow->value()));

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

  // Update the controlX/controlY
  OnControlPointUpdate();

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
    m_BoxCurve->GetInteractor()->SetMovingControlPoint(0);
    OnWindowLevelChange();
    }
  if (m_Curve->GetControlPointCount() == 3)
    m_BtnCurveLessControlPoint->deactivate();
}

void
LayerInspectorUILogic
::OnControlPointTextBoxUpdate()
{
  // Get the values of the control points
  int cp = m_BoxCurve->GetInteractor()->GetMovingControlPoint();
  if(cp >= 0)
    {
    // Get the range of the intensity
    double imin = m_GreyWrapper->GetImageMinNative();
    double imax = m_GreyWrapper->GetImageMaxNative();
    double delta = imax - imin;

    // Update the point, although the curve may reject these values
    float tnew = (m_InControlX->value() - imin) / delta;
    float xnew = m_InControlY->value();
    bool accept = m_BoxCurve->UpdateControlPoint((size_t) cp, tnew, xnew);

    // Update the text boxes with actual values
    if(!accept)
      {
      float told, xold;
      m_Curve->GetControlPoint(cp, told, xold);
      m_InControlX->value(told * delta + imin);
      m_InControlY->value(xold);
      }
    else
      {
      this->OnCurveChange();
      }
    }
}



void 
LayerInspectorUILogic
::OnControlPointUpdate()
{
  int cp = m_BoxCurve->GetInteractor()->GetMovingControlPoint();
  float fx, fy;
  m_Curve->GetControlPoint(cp, fx, fy);
  const double min = m_GreyWrapper->GetImageMinNative();
  const double max = m_GreyWrapper->GetImageMaxNative();
  const double delta = max - min;
  m_InControlX->value(fx*delta + min);
  m_InControlY->value(fy);
}

// Callbacks for the color map page
LayerInspectorUILogic::PresetInfo
LayerInspectorUILogic::m_PresetInfo[] = {
  {"Grayscale", 0x00ff0000},
  {"Jet", 0x00000000},
  {"Hot", 0x00000000},
  {"Cool", 0x00000000},
  {"Black to red", 0x00ff0000},
  {"Black to green", 0xff000000},
  {"Black to blue", 0xff000000},
  {"Spring", 0x00000000},
  {"Summer", 0x00000000},
  {"Autumn", 0x00000000},
  {"Winter", 0x00000000},
  {"Copper", 0x00000000},
  {"HSV", 0x00000000},
  {"Blue, white and red", 0x00000000},
  {"Red, white and blue", 0x00000000}};


void
LayerInspectorUILogic
::OnColorMapChange()
{
  m_GreyWrapper->SetColorMap(m_BoxColorMap->GetColorMap());
  m_GreyWrapper->UpdateIntensityMapFunction();
  this->OnColorMapSelectedPointUpdate();

  // Redraw parent
  m_Parent->RedrawWindows();
}

void
LayerInspectorUILogic
::SelectNextColorMap()
{
  if(m_InColorMapPreset->size() <= 1)
    return;

  // Cycle through the available colormaps
  int val = m_InColorMapPreset->value();
  int newval = ( (val < 0 ? 0 : val) + 1) % (m_InColorMapPreset->size() - 1);
  if(newval != val)
    m_InColorMapPreset->value(newval);
  this->OnColorMapPresetUpdate();
}

void 
LayerInspectorUILogic
::PopulateColorMapPresets()
{
  // Set the system presets
  m_InColorMapPreset->clear();
  for(int i = 0; i < ColorMap::COLORMAP_SIZE; i++)
    {
    if(i == ColorMap::COLORMAP_SIZE - 1)
      {
      string text = string("_") + m_PresetInfo[i].name;
      m_InColorMapPreset->add(text.c_str());
      }
    else
      {
      m_InColorMapPreset->add(m_PresetInfo[i].name.c_str());
      }
    }

  // Now add the custom presets
  std::vector<string> saved = 
    m_Driver->GetSystemInterface()->GetSavedObjectNames("ColorMaps");
  for(int i = 0; i < (int) saved.size(); i++)
    m_InColorMapPreset->add(saved[i].c_str());
}

void 
LayerInspectorUILogic
::OnColorMapPresetUpdate()
{
  ColorMap cm;
  m_BoxCurve->hide();

  // Apply the current preset
  int sel = m_InColorMapPreset->value();
  if(sel < 0)
    return;

  if(sel < ColorMap::COLORMAP_SIZE)
    {
    ColorMap::SystemPreset preset = 
      (ColorMap::SystemPreset) (ColorMap::COLORMAP_GREY + sel);
    
    // Update the color map
    cm.SetToSystemPreset(preset);
    }
  else
    {
    try 
      {
      Registry reg = 
        m_Driver->GetSystemInterface()->ReadSavedObject(
          "ColorMaps", m_InColorMapPreset->text(sel));
      cm.LoadFromRegistry(reg);
      }
    catch(IRISException &exc)
      {
      fl_alert("Failed to read preset from file: \n%s", exc.what());
      return;
      }    
    }

  m_BoxColorMap->SetColorMap(cm);
  m_BoxColorMap->SetSelectedCMPoint(-1);

  if(m_WinLayerUI->shown())
    {
    m_BoxColorMap->redraw();
    m_BoxColorMap->show();
    }

  // Update the image
  m_GreyWrapper->SetColorMap(cm);
  m_GreyWrapper->UpdateIntensityMapFunction();
  
  // Redraw parent
  m_Parent->RedrawWindows();

  this->OnColorMapSelectedPointUpdate();
}

void 
LayerInspectorUILogic
::OnColorMapAddPresetAction()
{
  // What default to recommend?
  int sel = m_InColorMapPreset->value();
  string deflt = (sel < ColorMap::COLORMAP_SIZE)
    ? string("My ") + m_InColorMapPreset->text(sel)
    : m_InColorMapPreset->text(sel);

  // Prompt the user for the name
  const char *name = fl_input("How do you want to name your preset?", deflt.c_str());
  if(!name || strlen(name) == 0)
    return;

  // Create a registry
  Registry reg;
  m_BoxColorMap->GetColorMap().SaveToRegistry(reg);

  // Write to file
  m_Driver->GetSystemInterface()->UpdateSavedObject("ColorMaps",name,reg);

  // Refresh the list of presents
  PopulateColorMapPresets();

  // Set the current preset as the selection
  m_InColorMapPreset->value(-1);
  for(int i = 0; i < m_InColorMapPreset->size(); i++)
    if(!strcmp(m_InColorMapPreset->text(i), name))
      { m_InColorMapPreset->value(i); break; }
}

void 
LayerInspectorUILogic
::OnColorMapDeletePresetAction()
{
  int sel = m_InColorMapPreset->value();
  if(sel >= ColorMap::COLORMAP_SIZE)
    {
    // Delete the bad one
    string name = m_InColorMapPreset->text(sel);
    m_Driver->GetSystemInterface()->DeleteSavedObject("ColorMaps",name.c_str());

    // Refresh the list of presents
    PopulateColorMapPresets();

    // Select the next color map
    if(sel < m_InColorMapPreset->size()-1)
      m_InColorMapPreset->value(sel);
    else
      m_InColorMapPreset->value(m_InColorMapPreset->size() - 2);
    this->OnColorMapPresetUpdate();
    }
}

void 
LayerInspectorUILogic
::OnColorMapSelectedPointUpdate()
{
  // Get the selected color map point
  int sel = m_BoxColorMap->GetSelectedCMPoint();
  ColorMapWidget::Side side = m_BoxColorMap->GetSelectedSide();

  if(sel >= 0)
    {
    // Activate relevant widgets
    m_InColorMapSide->activate();
    m_InColorMapIndex->activate();
    // m_InColorMapRGBA->activate();

    // Get color
    ColorMap &cm = this->m_BoxColorMap->GetColorMap();
    ColorMap::CMPoint p = cm.GetCMPoint(sel);

    // Handle continuous points
    if(p.m_Type == ColorMap::CONTINUOUS)
      {
      // Set the RGB value
      m_InColorMapRGBA->rgb(p.m_RGBA[0][0] / 255.0, p.m_RGBA[0][1] / 255.0, p.m_RGBA[0][2] / 255.0);

      // Set the location to 'both'
      m_InColorMapSide->value(0);
      }
    else if(side == ColorMapWidget::LEFT)
      {
      // Set the RGB value
      m_InColorMapRGBA->rgb(p.m_RGBA[0][0] / 255.0, p.m_RGBA[0][1] / 255.0, p.m_RGBA[0][2] / 255.0);

      // Set the location to 'both'
      m_InColorMapSide->value(1);
      }
    else if(side == ColorMapWidget::RIGHT)
      {
      // Set the RGB value
      m_InColorMapRGBA->rgb(p.m_RGBA[1][0] / 255.0, p.m_RGBA[1][1] / 255.0, p.m_RGBA[1][2] / 255.0);

      // Set the location to 'both'
      m_InColorMapSide->value(2);
      }

    // Set the index value
    m_InColorMapIndex->value(p.m_Index);

    // Set the max/min of the value
    if(sel == 0)
      {
      m_InColorMapIndex->minimum(0.0);
      m_InColorMapIndex->maximum(0.0);
      m_MenuColorMapBoth->deactivate();
      m_BtnColorMapDeletePoint->deactivate();
      }
    else if(sel == (int) cm.GetNumberOfCMPoints() - 1)
      {
      m_InColorMapIndex->minimum(1.0);
      m_InColorMapIndex->maximum(1.0);
      m_MenuColorMapBoth->deactivate();
      m_BtnColorMapDeletePoint->deactivate();
      }
    else
      {
      m_InColorMapIndex->minimum(cm.GetCMPoint(sel-1).m_Index);
      m_InColorMapIndex->maximum(cm.GetCMPoint(sel+1).m_Index);
      m_MenuColorMapBoth->activate();
      m_BtnColorMapDeletePoint->activate();
      }
    }
  else
    {
    m_BtnColorMapDeletePoint->deactivate();
    m_InColorMapSide->deactivate();
    m_InColorMapIndex->deactivate();
    // m_InColorMapRGBA->deactivate();
    }
}

void 
LayerInspectorUILogic
::OnColorMapIndexUpdate()
{
  int sel = this->m_BoxColorMap->GetSelectedCMPoint();
  if(sel >= 0)
    {
    m_InColorMapIndex->value(m_InColorMapIndex->clamp(m_InColorMapIndex->value()));
    ColorMap::CMPoint p = this->m_BoxColorMap->GetColorMap().GetCMPoint(sel);
    p.m_Index = m_InColorMapIndex->value();
    this->m_BoxColorMap->GetColorMap().UpdateCMPoint(sel, p);
    m_BoxColorMap->redraw();
    this->OnColorMapChange();
    }
}

void 
LayerInspectorUILogic
::OnColorMapSideUpdate()
{
  int sel = this->m_BoxColorMap->GetSelectedCMPoint();
  if(sel >= 0)
    {
    ColorMap &cm = this->m_BoxColorMap->GetColorMap();
    ColorMap::CMPoint p = cm.GetCMPoint(sel);
    size_t val = m_InColorMapSide->value();

    if(val == 0)
      {
      if(p.m_Type != ColorMap::CONTINUOUS)
        {
        // Make everything like the left
        p.m_Type = ColorMap::CONTINUOUS;
        p.m_RGBA[1] = p.m_RGBA[0];
        cm.UpdateCMPoint(sel, p);
        m_BoxColorMap->SetSelectedSide(ColorMapWidget::BOTH);
        m_BoxColorMap->redraw();
        this->OnColorMapChange();
        }
      }
    else
      {
      if(p.m_Type == ColorMap::CONTINUOUS)
        {
        // Split the point into two
        p.m_Type = ColorMap::DISCONTINUOUS;
        cm.UpdateCMPoint(sel, p);
        }
      m_BoxColorMap->SetSelectedSide(
        val == 1 ? ColorMapWidget::LEFT : ColorMapWidget::RIGHT);
      m_BoxColorMap->redraw();
      this->OnColorMapChange();
      }
    }
}

void 
LayerInspectorUILogic
::OnColorMapPointDelete()
{
  int sel = m_BoxColorMap->GetSelectedCMPoint();
  if(sel > 0 && sel < (int)(m_BoxColorMap->GetColorMap().GetNumberOfCMPoints() - 1))
    {
    m_BoxColorMap->SetSelectedCMPoint(-1);
    m_BoxColorMap->GetColorMap().DeleteCMPoint(sel);
    m_BoxColorMap->redraw();
    this->OnColorMapChange();
    }
}

void 
LayerInspectorUILogic
::OnColorMapRGBAUpdate()
{
  int sel = this->m_BoxColorMap->GetSelectedCMPoint();
  if(sel >= 0)
    {
    ColorMap::CMPoint p = this->m_BoxColorMap->GetColorMap().GetCMPoint(sel);
    if(p.m_Type == ColorMap::CONTINUOUS || m_InColorMapSide->value() == 1)
      {
      p.m_RGBA[0][0] = (unsigned char) (255 * m_InColorMapRGBA->r());
      p.m_RGBA[0][1] = (unsigned char) (255 * m_InColorMapRGBA->g());
      p.m_RGBA[0][2] = (unsigned char) (255 * m_InColorMapRGBA->b());
      }
    if(p.m_Type == ColorMap::CONTINUOUS || m_InColorMapSide->value() == 2)
      {
      p.m_RGBA[1][0] = (unsigned char) (255 * m_InColorMapRGBA->r());
      p.m_RGBA[1][1] = (unsigned char) (255 * m_InColorMapRGBA->g());
      p.m_RGBA[1][2] = (unsigned char) (255 * m_InColorMapRGBA->b());
      }

    this->m_BoxColorMap->GetColorMap().UpdateCMPoint(sel, p);
    m_BoxColorMap->redraw();
    this->OnColorMapChange();
    }

}

// Callbacks for the image info page
void
LayerInspectorUILogic
::UpdateImageProbe()
{
  // Code common to SNAP and IRIS
  Vector3ui crosshairs = m_Driver->GetCursorPosition();
  Vector3d xPosition = m_SelectedWrapper->TransformVoxelIndexToPosition(crosshairs);
  Vector3d xNIFTI = m_SelectedWrapper->TransformVoxelIndexToNIFTICoordinates(to_double(crosshairs));
  for (size_t d = 0; d < 3; ++d)
    {
    m_InImageInfoCursorIndex[d]->value(crosshairs[d]);
    m_OutImageInfoCursorPosition[d]->value(xPosition[d]);
    m_OutImageInfoCursorPosition[d]->maximum(
        m_OutImageInfoCursorPosition[d]->value());
    m_OutImageInfoCursorPosition[d]->minimum(
        m_OutImageInfoCursorPosition[d]->value());
    m_OutImageInfoCursorNIFTIPosition[d]->value(xNIFTI[d]);
    m_OutImageInfoCursorNIFTIPosition[d]->maximum(
        m_OutImageInfoCursorNIFTIPosition[d]->value());
    m_OutImageInfoCursorNIFTIPosition[d]->minimum(
        m_OutImageInfoCursorNIFTIPosition[d]->value());
    }

  if (m_GreyWrapper)
    {
    m_WizImageInfoVoxelValue->value(m_GrpWizImageInfoVoxelPageGray);
    m_OutImageInfoVoxelGray->value(m_GreyWrapper->GetVoxelMappedToNative(crosshairs));
    }
  else
    {
    m_WizImageInfoVoxelValue->value(m_GrpWizImageInfoVoxelPageRGB);
    RGBImageWrapper *rgb = dynamic_cast<RGBImageWrapper *>(m_SelectedWrapper);
    m_OutImageInfoVoxelRGB[0]->value(rgb->GetVoxel(crosshairs)[0]);
    m_OutImageInfoVoxelRGB[1]->value(rgb->GetVoxel(crosshairs)[1]);
    m_OutImageInfoVoxelRGB[2]->value(rgb->GetVoxel(crosshairs)[2]);
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

