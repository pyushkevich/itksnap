/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUILogic.h,v $
  Language:  C++
  Date:      $Date: 2011/04/18 15:06:07 $
  Version:   $Revision: 1.14 $
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
#include "ImageWrapper.h"

class UserInterfaceLogic;
class IRISApplication;
class GreyImageWrapper;

/**
 * \class LayerInspectorUILogic
 * \brief Logic class for Layer editor UI logic
 */
class LayerInspectorUILogic : public LayerInspectorUI
{
  typedef std::list<ImageWrapperBase *> WrapperList;
  typedef WrapperList::iterator WrapperIterator;
  typedef WrapperList::const_iterator WrapperConstIterator;

public:
  LayerInspectorUILogic(UserInterfaceLogic *);
  virtual ~LayerInspectorUILogic() {};

  // Initialization
  void Initialize();

  // Hook to the image wrappers
  void SetImageWrappers();

  // Callbacks for the main pane
  void OnLayerSelectionUpdate();
  void OnOverallOpacityUpdate();
  void OnCloseAction();

  // Callbacks for the contrast adjustment page
  void UpdateWindowAndLevel();
  void OnCurveChange();
  void OnCurveReset();
  void OnAutoFitWindow();
  void OnWindowLevelChange();
  void OnUpdateHistogram();
  void OnControlPointMoreAction();
  void OnControlPointLessAction();
  void OnControlPointTextBoxUpdate();
  void OnControlPointUpdate();

  // Callbacks for the color map page
  void OnColorMapChange();
  void OnColorMapPresetUpdate();
  void OnColorMapAddPresetAction();
  void OnColorMapDeletePresetAction();
  void OnColorMapIndexUpdate();
  void OnColorMapSideUpdate();
  void OnColorMapPointDelete();
  void OnColorMapRGBAUpdate();
  void OnColorMapSelectedPointUpdate();

  // Callbacks for the image info page
  void UpdateImageProbe();
  void OnImageInformationVoxelCoordinatesUpdate();

  // Display the dialog
  void DisplayWindow();
  void RedrawWindow();
  bool Shown();
  void DisplayImageContrastTab();
  void DisplayColorMapTab();
  void DisplayImageInfoTab();
  void DisplayImageAdvancedInfoTab();

  // External update to visibility of overlays
  void AdjustOverlayOpacity(double delta);
  void ToggleOverlayVisibility();

  // External update to colormap
  void SelectNextColorMap();

protected:

  void PopulateColorMapPresets();

  // Info about the presets
  struct PresetInfo 
    {
    std::string name;
    int cline, ccirc;
    };

  static PresetInfo m_PresetInfo[];

  // The intensity curve (same pointer stored in the m_BoxCurve)
  IntensityCurveInterface *m_Curve;

  // Pointer to the Parent GUI
  UserInterfaceLogic *m_Parent;

  // Pointer to the IRIS application object
  IRISApplication *m_Driver;

  // Main image wrapper and overlay wrapper lists
  ImageWrapperBase *m_MainWrapper;
  WrapperList *m_OverlayWrappers;

  // Currently selected wrapper
  ImageWrapperBase *m_SelectedWrapper;

  // Grey image wrapper for intensity curve
  GreyImageWrapper *m_GreyWrapper;
};

#endif
