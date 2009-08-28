/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUILogic.h,v $
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
#ifndef __LayerInspectorUILogic_h_
#define __LayerInspectorUILogic_h_

#include "LayerInspectorUI.h"

/**
 * \class LayerInspectorUILogic
 * \brief Logic class for Layer editor UI logic
 */
class LayerInspectorUILogic : public LayerInspectorUI
{
public:
  virtual ~LayerInspectorUILogic() {}

  // Callbacks for the main pane
  void OnLayerSelectionUpdate();
  void OnOverallOpacityUpdate();
  void OnCloseAction();

  // Callbacks for the contrast adjustment page
  void OnCurveReset();
  void OnAutoFitWindow();
  void OnWindowLevelChange();
  void OnControlPointNumberChange();
  void OnUpdateHistogram();
  void OnControlPointMoreAction();
  void OnControlPointLessAction();
  void OnCurveMakeLinearAction();
  void OnCurveMakeCubicAction();
  void OnControlPointUpdate();

  // Callbacks for the color map page
  void OnColorMapPresetUpdate();
  void OnColorMapAddPresetAction();
  void OnColorMapDeletePresetAction();
  void OnColorMapIndexUpdate();
  void OnColorMapSideUpdate();
  void OnColorMapPointDelete();
  void OnColorMapRGBAUpdate();

  // Callbacks for the image info page
  void OnImageInformationVoxelCoordinatesUpdate();

  // Display the dialog
  void DisplayWindow();

private:

  void PopulateColorMapPresets();

  // Info about the presets
  struct PresetInfo 
    {
    std::string name;
    int cline, ccirc;
    };

  static PresetInfo m_PresetInfo[];
};

#endif
