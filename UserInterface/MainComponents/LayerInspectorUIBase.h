/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LayerInspectorUIBase.h,v $
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
#ifndef __LayerInspectorUIBase_h_
#define __LayerInspectorUIBase_h_

/**
 * \class LayerInspectorUIBase
 * \brief Base class for Layer editor UI logic
 */
class LayerInspectorUIBase 
{
public:
  virtual ~LayerInspectorUIBase() {}

  // Callbacks for the main pane
  virtual void DisplayWindow() = 0;
  virtual void OnLayerSelectionUpdate() = 0;
  virtual void OnOverallOpacityUpdate() = 0;
  virtual void OnCloseAction() = 0;

  // Callbacks for the contrast adjustment page
  virtual void DisplayImageContrastTab() = 0;
  virtual void OnCurveReset() = 0;
  virtual void OnAutoFitWindow() = 0;
  virtual void OnWindowLevelChange() = 0;
  virtual void OnUpdateHistogram() = 0;
  virtual void OnControlPointMoreAction() = 0;
  virtual void OnControlPointLessAction() = 0;
  virtual void OnControlPointTextBoxUpdate() = 0;
  virtual void OnControlPointUpdate() = 0;

  // Callbacks for the color map page
  virtual void DisplayColorMapTab() = 0;
  virtual void OnColorMapPresetUpdate() = 0;
  virtual void OnColorMapAddPresetAction() = 0;
  virtual void OnColorMapDeletePresetAction() = 0;
  virtual void OnColorMapIndexUpdate() = 0;
  virtual void OnColorMapSideUpdate() = 0;
  virtual void OnColorMapPointDelete() = 0;
  virtual void OnColorMapRGBAUpdate() = 0;

  // Callbacks for the image info page
  virtual void DisplayImageInfoTab() = 0;
  virtual void OnImageInformationVoxelCoordinatesUpdate() = 0;
};

#endif
