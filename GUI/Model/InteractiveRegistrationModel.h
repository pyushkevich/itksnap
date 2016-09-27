/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: InteractiveRegistrationModel.h,v $
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
#ifndef INTERACTIVEREGISTRATIONMODEL_H
#define INTERACTIVEREGISTRATIONMODEL_H

#include <SNAPCommon.h>
#include <AbstractModel.h>

class GenericSliceModel;
class RegistrationModel;

class InteractiveRegistrationModel : public AbstractModel
{
public:
  irisITKObjectMacro(InteractiveRegistrationModel, AbstractModel)

  /**
   * The parent of this model is the main generic slice model, which
   * handles slice display and interaction
   */
  irisSetMacro(Parent, GenericSliceModel *)
  irisGetMacro(Parent, GenericSliceModel *)

  /**
   * The other parent of this model is the main registration model, which
   * is the back end of the registration dialog
   */
  irisSetMacro(RegistrationModel, RegistrationModel *)
  irisGetMacro(RegistrationModel, RegistrationModel *)


  bool ProcessPushEvent(const Vector3d &xSlice);
  bool ProcessDragEvent(const Vector3d &xSlice, const Vector3d &xDragStart);
  bool ProcessMouseMoveEvent(const Vector3d &xSlice, unsigned long hover_layer_id);
  bool ProcessReleaseEvent(const Vector3d &xSlice, const Vector3d &xDragStart);

  double GetRotationWidgetRadius();

  void RotateByTheta(double theta);

  irisIsMacro(HoveringOverMovingLayer)

  irisIsMacro(HoveringOverRotationWidget)

  irisGetMacro(LastTheta, double)

  /**
   * Check for a particular layer id whether the interactive registration widget
   * should be shown over that layer and whether mouse events over this layer are
   * processed as interactive registration events
   */
  bool GetDoProcessInteractionOverLayer(unsigned long layer_id);

protected:

  InteractiveRegistrationModel();
  ~InteractiveRegistrationModel();

  GenericSliceModel *m_Parent;
  RegistrationModel *m_RegistrationModel;

  // Radius of the rotation widget circle in slice coordinates
  double m_RotationWidgetRadius;

  // Whether we are hovering over the moving widget
  bool m_HoveringOverMovingLayer;

  // Whether we are hovering over the rotation widget radius (so it should be highlighted)
  bool m_HoveringOverRotationWidget;

  // During dragging, this stores the last theta rotation applied
  double m_LastTheta;

  // The last displacement vector
  Vector3d m_LastDisplacement;
};

#endif // INTERACTIVEREGISTRATIONMODEL_H
