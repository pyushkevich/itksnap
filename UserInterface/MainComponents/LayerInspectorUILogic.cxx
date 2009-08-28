/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/28 16:33:03 $
  Version:   $Revision: 1.1 $
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



void
LayerInspectorUILogic
::DisplayWindow()
{
  // Show the window
  m_WinLayerUI->show();

  // Initialize the color map
  ColorMap cm;
  cm.SetToSystemPreset(ColorMap::COLORMAP_RED);
  m_BoxColorMap->SetColorMap(cm);

  //curve = IntensityCurveVTK::New();
  //curve->Initialize(4);
  //m_BoxCurve->SetCurve(curve);

  // Show the GL stuff
  m_BoxColorMap->show();

  // Populate the preset chooser
  this->PopulateColorMapPresets();

  // m_BoxCurve->show();
}

// Callbacks for the main pane
void 
LayerInspectorUILogic
::OnLayerSelectionUpdate()
{

}


void 
LayerInspectorUILogic
::OnOverallOpacityUpdate()
{

}


void 
LayerInspectorUILogic
::OnCloseAction()
{

}



  // Callbacks for the contrast adjustment page
void 
LayerInspectorUILogic
::OnCurveReset()
{

}


void 
LayerInspectorUILogic
::OnAutoFitWindow()
{

}


void 
LayerInspectorUILogic
::OnWindowLevelChange()
{

}


void 
LayerInspectorUILogic
::OnControlPointNumberChange()
{

}


void 
LayerInspectorUILogic
::OnUpdateHistogram()
{

}


void 
LayerInspectorUILogic
::OnControlPointMoreAction()
{

}


void 
LayerInspectorUILogic
::OnControlPointLessAction()
{

}


void 
LayerInspectorUILogic
::OnCurveMakeLinearAction()
{

}


void 
LayerInspectorUILogic
::OnCurveMakeCubicAction()
{

}


void 
LayerInspectorUILogic
::OnControlPointUpdate()
{

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
  // Apply the current preset
  int sel = m_InColorMapPreset->value();
  ColorMap::SystemPreset preset = 
    (ColorMap::SystemPreset) (ColorMap::COLORMAP_GREY + sel);

  // TODO: this should affect the rest of SNAP
  ColorMap cm;
  cm.SetToSystemPreset(preset);
  m_BoxColorMap->SetColorMap(cm);
  m_BoxColorMap->redraw();
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
::OnImageInformationVoxelCoordinatesUpdate()
{

}



