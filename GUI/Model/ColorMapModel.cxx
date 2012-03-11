#include "ColorMapModel.h"
#include "LayerAssociation.txx"

// This compiles the LayerAssociation for the color map
template class LayerAssociation<ColorMapLayerProperties,
                                GreyImageWrapperBase,
                                ColorMapModelBase::PropertiesFactory>;

ColorMapModel::ColorMapModel()
{
  // Set up the models
  m_MovingControlPositionModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlPositionValueAndRange,
        &Self::SetMovingControlPosition);

  // Get the component model for opacity
  m_MovingControlOpacityModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlOpacityValueAndRange,
        &Self::SetMovingControlOpacity);

  m_MovingControlSideModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlSide,
        &Self::SetMovingControlSide);

  m_MovingControlContinuityModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlType,
        &Self::SetMovingControlType);

  m_MovingControlIndexModel = makeChildPropertyModel(
        this,
        &Self::GetMovingControlIndexValueAndRange,
        &Self::SetMovingControlIndex);

  // The model update events should also be rebroadcast as state changes
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

ColorMap* ColorMapModel::GetColorMap()
{
  return this->GetLayer()->GetColorMap();
}

bool
ColorMapModel
::IsControlSelected(int cp, Side side)
{
  ColorMap *cm = this->GetColorMap();
  ColorMapLayerProperties &p = this->GetProperties();

  if(p.GetSelectedControlIndex() == cp)
    {
    if(cm->GetCMPoint(cp).m_Type == ColorMap::CONTINUOUS)
      return true;
    else return p.GetSelectedControlSide() == side;
    }
  else return false;
}

bool
ColorMapModel
::SetSelection(int cp, Side side)
{
  ColorMap *cm = this->GetColorMap();
  ColorMapLayerProperties &p = this->GetProperties();
  int cp_current = p.GetSelectedControlIndex();
  Side side_current = p.GetSelectedControlSide();

  // Check if the control point has changed
  bool changed = (cp != cp_current || side != side_current);

  // Check the validity of the new selection
  if(cp >= 0)
    {
    bool disc = cm->GetCMPoint(cp).m_Type == ColorMap::DISCONTINUOUS;
    assert((disc && side != ColorMapLayerProperties::NA) ||
           (!disc && side == ColorMapLayerProperties::NA));
    }
  else
    {
    assert(side == ColorMapLayerProperties::NA);
    }

  // Set the new selection
  if(changed)
    {
    p.SetSelectedControlIndex(cp);
    p.SetSelectedControlSide(side);
    InvokeEvent(ModelUpdateEvent());
    }

  return changed;
}


void ColorMapModel::RegisterWithLayer(GreyImageWrapperBase *layer)
{
  unsigned long tag =
      Rebroadcast(layer->GetColorMap(),
                  ColorMapChangeEvent(), ModelUpdateEvent());
  GetProperties().SetObserverTag(tag);
}

void ColorMapModel::UnRegisterFromLayer(GreyImageWrapperBase *layer)
{
  unsigned long tag = GetProperties().GetObserverTag();
  if(tag)
    {
    layer->GetColorMap()->RemoveObserver(tag);
    }
}

bool ColorMapModel::ProcessMousePressEvent(const Vector3d &x)
{
  assert(m_ViewportReporter && m_ViewportReporter->CanReportSize());
  Vector2ui vp = m_ViewportReporter->GetViewportSize();

  // Reference to the color map
  ColorMap *cm = this->GetColorMap();

  // Check if the press occurs near a control point
  for(size_t i = 0; i < cm->GetNumberOfCMPoints(); i++)
    {
    ColorMap::CMPoint p = cm->GetCMPoint(i);
    double dx = fabs(x[0] - p.m_Index);
    double dy0 = fabs(x[1] - p.m_RGBA[0][3] / 255.0);
    double dy1 = fabs(x[1] - p.m_RGBA[1][3] / 255.0);

    if(dx / 1.2 < 5.0 / vp[0])
      {
      if(dy0 / 1.2 < 5.0 / vp[1])
        {
        // We return 0 when the selected point changes to avoid dragging
        if(p.m_Type == ColorMap::CONTINUOUS)
          this->SetSelection(i, ColorMapLayerProperties::NA);
        else
          this->SetSelection(i, ColorMapLayerProperties::LEFT);
        return 1;
        }
      else if (dy1 / 1.2 < 5.0 / vp[1])
        {
        this->SetSelection(i, ColorMapLayerProperties::RIGHT);
        return true;
        }
      }
    }

  // No selection has been made, so we insert a new point
  if(x[0] > 0.0 && x[0] < 1.0)
    {
    size_t sel = cm->InsertInterpolatedCMPoint(x[0]);
    this->SetSelection(sel, ColorMapLayerProperties::NA);
    return true;
    }

  return false;
}

bool ColorMapModel::ProcessMouseDragEvent(const Vector3d &x)
{
  // Reference to the color map
  ColorMap *cm = this->GetColorMap();
  ColorMapLayerProperties &p  = this->GetProperties();
  int isel = p.GetSelectedControlIndex();
  Side side = p.GetSelectedControlSide();

  // Nothing happens if zero is selected
  if(isel < 0) return false;

  // Get the selected point
  ColorMap::CMPoint psel = cm->GetCMPoint(isel);

  // Get the new alpha and index
  double j = x[0];
  double a = x[1] * 255;

  // Clip the new index
  if(isel == 0 || isel == (int)cm->GetNumberOfCMPoints()-1)
    {
    // The first and last point can not be moved left or right
    j = psel.m_Index;
    }
  else
    {
    // Other points are constrained by neighbors
    ColorMap::CMPoint p0 = cm->GetCMPoint(isel-1);
    ColorMap::CMPoint p1 = cm->GetCMPoint(isel+1);
    if(j < p0.m_Index) j = p0.m_Index;
    if(j > p1.m_Index) j = p1.m_Index;
    }

  // Update the index of the point
  psel.m_Index = j;

  // Clip the new alpha
  if(a < 0) a = 0;
  if(a > 255) a = 255;

  // Assign the alpha
  if(side != ColorMapLayerProperties::RIGHT)
    psel.m_RGBA[0][3] = (unsigned char) a;
  if(side != ColorMapLayerProperties::LEFT)
    psel.m_RGBA[1][3] = (unsigned char) a;

  // Redraw
  cm->UpdateCMPoint(isel, psel);

  return true;
}

bool ColorMapModel::ProcessMouseReleaseEvent(const Vector3d &x)
{
  return true;
}

void ColorMapModel::OnUpdate()
{
  if(m_EventBucket->HasEvent(ColorMapChangeEvent()))
    {
    this->GetLayer()->UpdateIntensityMapFunction();
    }
}

bool
ColorMapModel
::GetMovingControlPositionValueAndRange(
    double &value, NumericValueRange<double> *range)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  if(idx >= 0)
    {
    value = cmap->GetCMPoint(idx).m_Index;
    if(range)
      {
      range->StepSize = 0.001;
      if(idx == 0)
        {
        range->Minimum = 0.0;
        range->Maximum = 0.0;
        }
      else if (idx == (int) cmap->GetNumberOfCMPoints()-1)
        {
        range->Minimum = 1.0;
        range->Maximum = 1.0;
        }
      else
        {
        range->Minimum = cmap->GetCMPoint(idx - 1).m_Index;
        range->Maximum = cmap->GetCMPoint(idx + 1).m_Index;
        }
      }
    return true;
    }
  else return false;
}

void
ColorMapModel
::SetMovingControlPosition(double value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  assert(idx >= 0);

  ColorMap::CMPoint pt = cmap->GetCMPoint(idx);
  pt.m_Index = value;
  cmap->UpdateCMPoint(idx, pt);
}



bool ColorMapModel::GetSelectedRGBA(ColorMap::RGBAType &rgba)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  Side side = p.GetSelectedControlSide();

  if(idx >= 0)
    {
    ColorMap::CMPoint pt = cmap->GetCMPoint(idx);
    int iside = (pt.m_Type == ColorMap::DISCONTINUOUS &&
                 side == ColorMapLayerProperties::RIGHT) ? 1 : 0;
    rgba = pt.m_RGBA[iside];
    return true;
    }
  else return false;
}

void ColorMapModel::SetSelectedRGBA(ColorMap::RGBAType rgba)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  Side side = p.GetSelectedControlSide();
  assert(idx >= 0);

  // Assign to left, right, or both sides
  ColorMap::CMPoint pt = cmap->GetCMPoint(idx);
  if(pt.m_Type == ColorMap::CONTINUOUS || side == ColorMapLayerProperties::LEFT)
    pt.m_RGBA[0] = rgba;
  if(pt.m_Type == ColorMap::CONTINUOUS || side == ColorMapLayerProperties::RIGHT)
    pt.m_RGBA[1] = rgba;

  cmap->UpdateCMPoint(idx, pt);
}

Vector3d ColorMapModel::GetSelectedColor()
{
  ColorMap::RGBAType rgba;
  if(this->GetSelectedRGBA(rgba))
    return Vector3d(rgba[0] / 255., rgba[1] / 255., rgba[2] / 255.);
  else
    return Vector3d(0,0,0);
}


void ColorMapModel::SetSelectedColor(Vector3d rgb)
{
  ColorMap::RGBAType rgba;
  if(this->GetSelectedRGBA(rgba))
    {
    rgba[0] = (unsigned char)(255.0 * rgb[0]);
    rgba[1] = (unsigned char)(255.0 * rgb[1]);
    rgba[2] = (unsigned char)(255.0 * rgb[2]);
    this->SetSelectedRGBA(rgba);
    }
}


bool
ColorMapModel
::GetMovingControlOpacityValueAndRange(
    double &value, NumericValueRange<double> *range)
{
  ColorMap::RGBAType rgba;
  if(this->GetSelectedRGBA(rgba))
    {
    value = rgba[3] / 255.0;
    if(range)
      {
      range->Set(0.0, 1.0, 0.01);
      }
    return true;
    }
  else return false;
}

void
ColorMapModel
::SetMovingControlOpacity(double value)
{
  ColorMap::RGBAType rgba;
  if(this->GetSelectedRGBA(rgba))
    {
    rgba[3] = (unsigned char)(255.0 * value);
    this->SetSelectedRGBA(rgba);
    }
}


bool
ColorMapModel
::CheckState(ColorMapModel::UIState state)
{
  // All flags are false if no layer is loaded
  if(this->GetLayer() == NULL)
    return false;

  // Otherwise get the properties
  ColorMapLayerProperties &p = this->GetProperties();
  int idx = p.GetSelectedControlIndex();
  int npts = (int) this->GetColorMap()->GetNumberOfCMPoints();

  switch(state)
    {
    case UIF_LAYER_ACTIVE:
      return true;
    case UIF_CONTROL_SELECTED:
      return idx >= 0;
    case UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT:
      return idx > 0 && idx < npts - 1;
    case UIF_CONTROL_SELECTED_IS_DISCONTINUOUS:
      return idx >= 0 && this->GetColorMap()->GetCMPoint(idx).m_Type ==
          ColorMap::DISCONTINUOUS;
    case UIF_USER_PRESET_SELECTED:
      return find(m_PresetUser.begin(), m_PresetUser.end(),
                  this->GetProperties().GetSelectedPreset()) < m_PresetUser.end();
    }
  return false;
}

bool ColorMapModel
::GetMovingControlSide(Side &value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  int idx = p.GetSelectedControlIndex();

  if(idx >= 0)
    {
    value = p.GetSelectedControlSide();
    return true;
    }
  return false;
}

void
ColorMapModel
::SetMovingControlSide(Side value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  assert(idx >= 0);
  ColorMap::CMPoint pt = cmap->GetCMPoint(idx);
  assert(pt.m_Type == ColorMap::DISCONTINUOUS);
  p.SetSelectedControlSide(value);
  InvokeEvent(ModelUpdateEvent());
}

bool ColorMapModel::GetMovingControlType(Continuity &value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();

  if(idx >= 0)
    {
    value = cmap->GetCMPoint(idx).m_Type;
    return true;
    }
  else return false;
}

void ColorMapModel::SetMovingControlType(Continuity value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  Side side = p.GetSelectedControlSide();
  assert(idx >= 0);
  ColorMap::CMPoint pt = cmap->GetCMPoint(idx);

  if(value != pt.m_Type)
    {
    pt.m_Type = value;
    if(value == ColorMap::CONTINUOUS)
      {
      p.SetSelectedControlSide(ColorMapLayerProperties::NA);
      if(side == ColorMapLayerProperties::LEFT)
        pt.m_RGBA[1] = pt.m_RGBA[0];
      else
        pt.m_RGBA[0] = pt.m_RGBA[1];
      }
    else
      {
      p.SetSelectedControlSide(ColorMapLayerProperties::LEFT);
      }

    cmap->UpdateCMPoint(idx, pt);
    InvokeEvent(ModelUpdateEvent());
    }
}


void ColorMapModel::GetPresets(
    ColorMapModel::PresetList &system, ColorMapModel::PresetList &user)
{
  system = m_PresetSystem;
  user = m_PresetUser;
}

void ColorMapModel::SelectPreset(const char *preset)
{
  // Is this a system preset?
  if(preset && strlen(preset))
    {
    // Is it a system preset?
    PresetList::iterator itSys =
        find(m_PresetSystem.begin(), m_PresetSystem.end(), string(preset));
    if(itSys != m_PresetSystem.end())
      {
      this->GetColorMap()->SetToSystemPreset(
            static_cast<ColorMap::SystemPreset>(itSys - m_PresetSystem.begin()));
      }
    else
      {
      try
        {
        Registry reg = m_System->ReadSavedObject("ColorMaps", preset);
        this->GetColorMap()->LoadFromRegistry(reg);
        }
      catch(IRISException &exc)
        {
        throw IRISException("Unable to read preset from disk. Exception: %s",
                            exc.what());
        }
      }
    }

  // Store the selected preset
  this->GetProperties().SetSelectedPreset(preset);

  // Clear the selection
  this->GetProperties().SetSelectedControlIndex(-1);
  this->GetProperties().SetSelectedControlSide(ColorMapLayerProperties::NA);
  this->InvokeEvent(ModelUpdateEvent());
}

void ColorMapModel::SetParentModel(GlobalUIModel *parent)
{
  Superclass::SetParentModel(parent);

  // Get the pointer to the system interface
  m_System = m_ParentModel->GetDriver()->GetSystemInterface();

  // Get the list of system presets
  for(int i = 0; i < ColorMap::COLORMAP_SIZE; i++)
    {
    m_PresetSystem.push_back(
          ColorMap::GetPresetName(static_cast<ColorMap::SystemPreset>(i)));
    }

  // Get the current list of user presets. I am assuming that this will not
  // change other than through the model
  m_PresetUser = m_System->GetSavedObjectNames("ColorMaps");
}

#include <algorithm>

void ColorMapModel::SaveAsPreset(std::string name)
{
  // Put the colormap in a registry
  Registry reg;
  this->GetColorMap()->SaveToRegistry(reg);

  // Write to file
  m_System->UpdateSavedObject("ColorMaps", name.c_str(), reg);

  // Refresh the list of user presets
  m_PresetUser = m_System->GetSavedObjectNames("ColorMaps");

  // Find the new preset in the list
  PresetList::iterator it =
      find(m_PresetUser.begin(), m_PresetUser.end(), name);

  if(it != m_PresetUser.end())
    {
    this->GetProperties().SetSelectedPreset(name);
    }
  else
    {
    this->GetProperties().SetSelectedPreset("");
    }

  // Invoke an update event
  InvokeEvent(PresetUpdateEvent());
}

void ColorMapModel::DeletePreset(std::string name)
{
  // Delete the preset
  m_System->DeleteSavedObject("ColorMaps",name.c_str());

  // Clear the selected preset
  GetProperties().SetSelectedPreset("");

  // Update the user preset list
  m_PresetUser = m_System->GetSavedObjectNames("ColorMaps");


  // Fire an event
  InvokeEvent(PresetUpdateEvent());
}

bool ColorMapModel::GetMovingControlIndexValueAndRange(
    int &value, NumericValueRange<int> *range)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  int idx = p.GetSelectedControlIndex();
  if(idx >= 0)
    {
    value = idx + 1;
    if(range)
      range->Set(1, cmap->GetNumberOfCMPoints(), 1);
    return true;
    }
  return false;
}

void ColorMapModel::SetMovingControlIndex(int value)
{
  ColorMapLayerProperties &p = this->GetProperties();
  ColorMap *cmap = this->GetColorMap();
  ColorMap::CMPoint pt = cmap->GetCMPoint(value - 1);

  Side newside = ColorMapLayerProperties::NA;
  if(pt.m_Type == ColorMap::DISCONTINUOUS)
    {
    // Pick the side that is closest to the current selection
    if(value < p.GetSelectedControlIndex())
      newside = ColorMapLayerProperties::RIGHT;
    else
      newside = ColorMapLayerProperties::LEFT;
    }

  this->SetSelection(value - 1, newside);
}

void ColorMapModel::DeleteSelectedControl()
{
  ColorMapLayerProperties &p = this->GetProperties();
  int sel = p.GetSelectedControlIndex();
  ColorMap *cmap = this->GetColorMap();

  if(sel > 0 && sel < (int)(cmap->GetNumberOfCMPoints() - 1))
    {
    // Delete the point
    cmap->DeleteCMPoint(sel);

    // Update the selection
    this->SetMovingControlIndex(sel);
    }
}






