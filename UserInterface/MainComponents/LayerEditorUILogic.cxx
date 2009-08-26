/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerEditorUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/26 21:49:55 $
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

#include "LayerEditorUILogic.h"
#include "IntensityCurveVTK.h"



void
LayerEditorUILogic
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
LayerEditorUILogic
::OnLayerSelectionUpdate()
{

}


void 
LayerEditorUILogic
::OnOverallOpacityUpdate()
{

}


void 
LayerEditorUILogic
::OnCloseAction()
{

}



  // Callbacks for the contrast adjustment page
void 
LayerEditorUILogic
::OnCurveReset()
{

}


void 
LayerEditorUILogic
::OnAutoFitWindow()
{

}


void 
LayerEditorUILogic
::OnWindowLevelChange()
{

}


void 
LayerEditorUILogic
::OnControlPointNumberChange()
{

}


void 
LayerEditorUILogic
::OnUpdateHistogram()
{

}


void 
LayerEditorUILogic
::OnControlPointMoreAction()
{

}


void 
LayerEditorUILogic
::OnControlPointLessAction()
{

}


void 
LayerEditorUILogic
::OnCurveMakeLinearAction()
{

}


void 
LayerEditorUILogic
::OnCurveMakeCubicAction()
{

}


void 
LayerEditorUILogic
::OnControlPointUpdate()
{

}



// Callbacks for the color map page
LayerEditorUILogic::PresetInfo
LayerEditorUILogic::m_PresetInfo[] = {
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
LayerEditorUILogic
::PopulateColorMapPresets()
{
  m_InColorMapPreset->clear();
  for(int i = 0; i < ColorMap::COLORMAP_SIZE; i++)
    m_InColorMapPreset->add(m_PresetInfo[i].name.c_str());
}


void 
LayerEditorUILogic
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
LayerEditorUILogic
::OnColorMapAddPresetAction()
{

}


void 
LayerEditorUILogic
::OnColorMapDeletePresetAction()
{

}


void 
LayerEditorUILogic
::OnColorMapIndexUpdate()
{

}


void 
LayerEditorUILogic
::OnColorMapSideUpdate()
{

}


void 
LayerEditorUILogic
::OnColorMapPointDelete()
{

}


void 
LayerEditorUILogic
::OnColorMapRGBAUpdate()
{

}



  // Callbacks for the image info page
void 
LayerEditorUILogic
::OnImageInformationVoxelCoordinatesUpdate()
{

}



