#ifndef COLORMAPPRESETMANAGER_H
#define COLORMAPPRESETMANAGER_H

#include "ColorMap.h"
#include "PresetManager.h"

/**
 * @brief The traits class describing ColorMap for preset management
 */
class ColorMapPresetTraits
{
public:
  typedef ColorMap ManagedType;
  typedef ColorMap::SystemPreset SystemPresetIterator;

  static std::string GetPresetCategoryName() { return std::string("ColorMaps"); }

  static SystemPresetIterator SystemPresetBegin() { return ColorMap::COLORMAP_GREY; }

  static SystemPresetIterator SystemPresetEnd() { return ColorMap::COLORMAP_CUSTOM; }

  static void SetToSystemPreset(ManagedType *cmap, const SystemPresetIterator &preset)
    { cmap->SetToSystemPreset(preset); }

  static std::string GetSystemPresetName(const SystemPresetIterator &preset)
    { return ColorMap::GetPresetName(preset); }

  static void ReadFromRegistry(ManagedType *cmap, Registry &folder)
    { cmap->LoadFromRegistry(folder); }

  static void WriteToRegistry(ManagedType *cmap, Registry &folder)
    { cmap->SaveToRegistry(folder); }
};

// The color map preset manager
typedef PresetManager<ColorMapPresetTraits> ColorMapPresetManager;

#endif // COLORMAPPRESETMANAGER_H
