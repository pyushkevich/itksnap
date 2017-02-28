/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: InteractiveRegistrationModel.cxx,v $
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
#include "InteractiveRegistrationModel.h"
#include "GenericSliceModel.h"
#include "RegistrationModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"

double InteractiveRegistrationModel::GetRotationWidgetRadius()
{
  RegistrationModel *rmodel = this->GetRegistrationModel();
  GenericSliceModel *smodel = this->GetParent();
  ImageWrapperBase *moving = rmodel->GetMovingLayerWrapper();

  if(!moving)
    return 0;

  // Get the center of rotation
  Vector3ui rot_ctr_image = rmodel->GetRotationCenter();

  // Map the center of rotation into the slice coordinates
  Vector3d rot_ctr_slice = smodel->MapImageToSlice(to_double(rot_ctr_image));

  // Get the corners of the viewport in slice coordinate units
  const SliceViewportLayout::SubViewport &vp =
      smodel->GetViewportLayout().vpList.front();

  // We will use a much simplified way of computing the arc. Taking eight points
  // at the compass directions of the viewport, we will find the one farthest away
  // from the rotation center, and draw a circle through it
  int margin = 10;
  Vector2ui u0(margin, margin);
  Vector2ui u1(vp.size[0] / 2, vp.size[1] / 2);
  Vector2ui u2(vp.size[0] - margin, vp.size[1] - margin);

  // Vector2ui u0(vp.pos[0] + margin, vp.pos[1] + margin);
  // Vector2ui u1(vp.pos[0] + vp.size[0] / 2, vp.pos[1] + vp.size[1] / 2);
  // Vector2ui u2(vp.pos[0] + vp.size[0] - margin, vp.pos[1] + vp.size[1] - margin);

  std::vector<Vector2ui> xProbe;
  xProbe.push_back(Vector2ui(u0[0], u0[1]));
  xProbe.push_back(Vector2ui(u0[0], u1[1]));
  xProbe.push_back(Vector2ui(u0[0], u2[1]));
  xProbe.push_back(Vector2ui(u1[0], u2[1]));
  xProbe.push_back(Vector2ui(u2[0], u2[1]));
  xProbe.push_back(Vector2ui(u2[0], u1[1]));
  xProbe.push_back(Vector2ui(u2[0], u0[1]));
  xProbe.push_back(Vector2ui(u1[0], u0[1]));

  double r = 0;
  for(int i = 0; i < xProbe.size(); i++)
    {
    Vector3d x_probe_slice = smodel->MapWindowToSlice(to_double(xProbe[i]));
    double dx = (x_probe_slice[0] - rot_ctr_slice[0]) * smodel->GetSliceSpacing()[0];
    double dy = (x_probe_slice[1] - rot_ctr_slice[1]) * smodel->GetSliceSpacing()[1];
    double rtest = sqrt(dx * dx + dy * dy);
    if(rtest > r)
      r = rtest;
    }

  // There we have it, we found the r that we want.
  return r;
}

void InteractiveRegistrationModel::RotateByTheta(double theta)
{
  // Apply the rotation angle to the transform. To do this, we use the axis-angle rotation
  // formulation. The axis, in this case, is the normal to the slice in physical space of
  // the reference image.
  RegistrationModel *rmodel = this->GetRegistrationModel();
  GenericSliceModel *smodel = this->GetParent();

  // Map a vector in the z direction into image coordinates
  Vector3d v_image = to_double(smodel->GetDisplayToImageTransform()->TransformVector(Vector3d(0, 0, -1)));

  // Transform this vector into physical space
  itk::Vector<double, 3> axis_image, axis_phys;
  axis_image.SetVnlVector(v_image);
  smodel->GetDriver()->GetCurrentImageData()->GetMain()->GetImageBase()
      ->TransformLocalVectorToPhysicalVector(axis_image, axis_phys);

  Vector3d v_phys = axis_phys.GetVnlVector();
  v_phys.normalize();

  // Now we have an actual axis of rotation
  rmodel->ApplyRotation(v_phys, theta);
}

InteractiveRegistrationModel::InteractiveRegistrationModel()
  : m_HoveringOverRotationWidget(false), m_HoveringOverMovingLayer(false)
{

}

InteractiveRegistrationModel::~InteractiveRegistrationModel()
{

}


bool InteractiveRegistrationModel::ProcessPushEvent(const Vector3d &xSlice)
{
  if(m_HoveringOverRotationWidget)
    {
    m_LastTheta = 0;
    return true;
    }
  else if(m_HoveringOverMovingLayer)
    {
    m_LastDisplacement = 0;
    return true;
    }
  else
    {
    // Don't handle events occurring over fixed / other layers, so that the user
    // can still move the cursor
    return false;
    }
}

bool InteractiveRegistrationModel::ProcessDragEvent(const Vector3d &xSlice, const Vector3d &xDragStart)
{
  // The user is moving the mouse. We just want to check if we are close to the rotation widget
  RegistrationModel *rmodel = this->GetRegistrationModel();
  GenericSliceModel *smodel = this->GetParent();
  ImageWrapperBase *moving = rmodel->GetMovingLayerWrapper();

  if(!moving || !m_HoveringOverMovingLayer)
    return false;

  if(m_HoveringOverRotationWidget)
    {
    // Treat this as a rotation event

    // Get the center of rotation
    Vector3ui rot_ctr_image = rmodel->GetRotationCenter();

    // Map the center of rotation into the slice coordinates
    Vector3d rot_ctr_slice = smodel->MapImageToSlice(to_double(rot_ctr_image));

    // Compute the rotation angle
    double dx0 = (xDragStart[0] - rot_ctr_slice[0]) * smodel->GetSliceSpacing()[0];
    double dy0 = (xDragStart[1] - rot_ctr_slice[1]) * smodel->GetSliceSpacing()[1];
    double dx1 = (xSlice[0] - rot_ctr_slice[0]) * smodel->GetSliceSpacing()[0];
    double dy1 = (xSlice[1] - rot_ctr_slice[1]) * smodel->GetSliceSpacing()[1];

    double theta = atan2(dy1, dx1) - atan2(dy0, dx0);

    this->RotateByTheta(theta - m_LastTheta);

    // Set the last theta
    m_LastTheta = theta;
    }
  else
    {
    // Find the physical coordinates of the start and end points of the drag
    Vector3d xDrag2 = smodel->MapSliceToImagePhysical(xSlice);
    Vector3d xDrag1 = smodel->MapSliceToImagePhysical(xDragStart);

    // Total displacement so far
    Vector3d xDispTotal = xDrag2 - xDrag1;

    // Apply the delta displacement
    rmodel->ApplyTranslation(xDispTotal - m_LastDisplacement);

    // Store the displacement
    m_LastDisplacement = xDispTotal;
    }

  return true;
}

bool
InteractiveRegistrationModel
::GetDoProcessInteractionOverLayer(unsigned long layer_id)
{
  RegistrationModel *rmodel = this->GetRegistrationModel();
  ImageWrapperBase *moving = rmodel->GetMovingLayerWrapper();

  // If tiling and moving layer is drawing
  // If stacked and moving layer is drawing
  // If moving layer is an overlay
  return moving && (moving->GetUniqueId() == layer_id || moving->IsSticky());
}

bool
InteractiveRegistrationModel
::ProcessMouseMoveEvent(
    const Vector3d &xSlice, unsigned long hover_layer_id)
{
  // The user is moving the mouse. We just want to check if we are close to the rotation widget
  RegistrationModel *rmodel = this->GetRegistrationModel();
  GenericSliceModel *smodel = this->GetParent();

  // Does this interaction fall into our provenance?
  if(!this->GetDoProcessInteractionOverLayer(hover_layer_id))
    {
    m_HoveringOverMovingLayer = false;
    m_HoveringOverRotationWidget = false;
    return false;
    }

  // We are hovering over the moving layer
  m_HoveringOverMovingLayer = true;

  // Get the center of rotation
  Vector3ui rot_ctr_image = rmodel->GetRotationCenter();

  // Map the center of rotation into the slice coordinates
  Vector3d rot_ctr_slice = smodel->MapImageToSlice(to_double(rot_ctr_image));

  // Check the distance to the widget
  double radius = this->GetRotationWidgetRadius() / 2.0;

  double dx = (xSlice[0] - rot_ctr_slice[0]) * smodel->GetSliceSpacing()[0];
  double dy = (xSlice[1] - rot_ctr_slice[1]) * smodel->GetSliceSpacing()[1];
  double rtest = sqrt(dx * dx + dy * dy);

  bool newHover = fabs(rtest - radius) < 0.1 * radius;
  if(newHover != m_HoveringOverRotationWidget)
    {
    m_HoveringOverRotationWidget = newHover;
    InvokeEvent(ModelUpdateEvent());
    }

  return true;
}

bool InteractiveRegistrationModel::ProcessReleaseEvent(const Vector3d &xSlice, const Vector3d &xDragStart)
{
  bool status = this->ProcessDragEvent(xSlice, xDragStart);
  m_LastTheta = 0;
  return status;
}

