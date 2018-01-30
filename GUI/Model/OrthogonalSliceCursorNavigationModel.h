/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#ifndef ORTHOGONALSLICECURSORNAVIGATIONMODEL_H
#define ORTHOGONALSLICECURSORNAVIGATIONMODEL_H

#include <SNAPCommon.h>
#include "AbstractModel.h"

class GenericSliceModel;

class OrthogonalSliceCursorNavigationModel : public AbstractModel
{
public:

  irisITKObjectMacro(OrthogonalSliceCursorNavigationModel, AbstractModel)

  irisSetMacro(Parent, GenericSliceModel *)
  irisGetMacro(Parent, GenericSliceModel *)

  // Move 3D cursor to (x,y) point on the screen supplied by user
  void UpdateCursor(Vector2f x);

  // Start zoom operation
  void BeginZoom();

  // End zoom operation
  void EndZoom();

  // Start pan operation
  void BeginPan();

  // End pan
  void EndPan();

  // Process zoom or pan operation (parameter is the gesture length,
  // which in theory should work both on desktop and on an iPhone).
  void ProcessZoomGesture(float scaleFactor);

  // Process pan operation (parameter is the gesture vector, i.e., mouse
  // drag or three-finger gesture)
  void ProcessPanGesture(Vector2f uvOffset);

  // Process a scrolling-type gesture (mouse wheel, or two-finger scroll)
  void ProcessScrollGesture(float gLength);

  // Check if the user press position is inside the thumbnail
  bool CheckZoomThumbnail(Vector2i xCanvas);

  // Process pan operation (parameter is the gesture vector, i.e., mouse
  // drag or three-finger gesture)
  void ProcessThumbnailPanGesture(Vector2i uvOffset);

  // Process arrow key and pageup/down commands
  void ProcessKeyNavigation(Vector3i dx);
protected:

  OrthogonalSliceCursorNavigationModel() {}
  ~OrthogonalSliceCursorNavigationModel() {}

  // Zoom and pan factors at the beginning of interaction
  Vector2f m_StartViewPosition;
  float m_StartViewZoom;

  GenericSliceModel *m_Parent;


};

#endif // ORTHOGONALSLICECURSORNAVIGATIONMODEL_H
