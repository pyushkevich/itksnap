#include "MultiChannelDisplayMode.h"


/* ===============================================================
    MultiChannelDisplayMode implementation
   =============================================================== */
MultiChannelDisplayMode::MultiChannelDisplayMode()
{
  UseRGB = false;
  RenderAsGrid = false;
  SelectedScalarRep = SCALAR_REP_COMPONENT;
  SelectedComponent = 0;
}

MultiChannelDisplayMode::MultiChannelDisplayMode(
    bool use_rgb, bool render_as_grid,
    ScalarRepresentation rep,
    int comp)
  : UseRGB(use_rgb), RenderAsGrid(render_as_grid),
    SelectedScalarRep(rep), SelectedComponent(comp)
{
}

MultiChannelDisplayMode::MultiChannelDisplayMode(int value)
{
  UseRGB = false;
  RenderAsGrid = false;
  SelectedScalarRep = SCALAR_REP_COMPONENT;
  SelectedComponent = 0;
}

MultiChannelDisplayMode
MultiChannelDisplayMode::DefaultForRGB()
{
  MultiChannelDisplayMode mode;
  mode.UseRGB = true;
  return mode;
}

void MultiChannelDisplayMode::Save(Registry &reg) const
{
  reg["UseRGB"] << UseRGB;
  reg["RenderAsGrid"] << RenderAsGrid;
  reg["SelectedScalarRep"].PutEnum(GetScalarRepNames(), SelectedScalarRep);
  reg["SelectedComponent"] << SelectedComponent;
}

MultiChannelDisplayMode
MultiChannelDisplayMode::Load(Registry &reg)
{
  MultiChannelDisplayMode mode;
  mode.UseRGB = reg["UseRGB"][mode.UseRGB];
  mode.RenderAsGrid = reg["RenderAsGrid"][mode.RenderAsGrid];
  mode.SelectedScalarRep = reg["SelectedScalarRep"].GetEnum(
        GetScalarRepNames(), mode.SelectedScalarRep);
  mode.SelectedComponent = reg["SelectedComponent"][mode.SelectedComponent];
  return mode;
}

RegistryEnumMap<ScalarRepresentation> &
MultiChannelDisplayMode::GetScalarRepNames()
{
  static RegistryEnumMap<ScalarRepresentation> namemap;
  if(namemap.Size() == 0)
    {
    namemap.AddPair(SCALAR_REP_COMPONENT, "Component");
    namemap.AddPair(SCALAR_REP_MAGNITUDE, "Magnitude");
    namemap.AddPair(SCALAR_REP_MAX, "Maximum");
    namemap.AddPair(SCALAR_REP_AVERAGE, "Average");
    }
  return namemap;
}

int MultiChannelDisplayMode::GetHashValue() const
{
  if(RenderAsGrid)
    return 0x1000000;

  if(UseRGB)
    return 0x8000000;

  if(SelectedScalarRep != SCALAR_REP_COMPONENT)
    return 0x4000000 + SelectedScalarRep;

  return SelectedComponent;
}

bool MultiChannelDisplayMode::IsSingleComponent()
{
  return !UseRGB && !RenderAsGrid && (SelectedScalarRep == SCALAR_REP_COMPONENT);
}

bool operator < (const MultiChannelDisplayMode &a, const MultiChannelDisplayMode &b)
{
  return a.GetHashValue() < b.GetHashValue();
}

