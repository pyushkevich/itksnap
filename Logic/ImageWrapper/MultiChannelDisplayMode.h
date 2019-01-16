#ifndef MULTICHANNELDISPLAYMODE_H
#define MULTICHANNELDISPLAYMODE_H

#include "ImageWrapperBase.h"
#include "Registry.h"

/**
 * This little struct describes the behavior of the multichannel display
 * mapping policy
 */
struct MultiChannelDisplayMode
{
  /**
   * Whether or not components are mapped to RGB channels. This is only
   * allowed if the image has 3 components
   */
  bool UseRGB;

  /**
   * Special mode for rendering 3-component images as displacement grids
   */
  bool RenderAsGrid;

  /**
   * When not in RGB mode, which scalar representation is selected for
   * display. Only used if UseRGB is false.
   */
  ScalarRepresentation SelectedScalarRep;

  /**
   * When the scalar representation is 'component', which component is
   * selected for display.
   */
  int SelectedComponent;

  /** Default constructor - select first component */
  MultiChannelDisplayMode();

  /** Default constructor - select first component */
  MultiChannelDisplayMode(bool use_rgb, bool render_as_grid,
                          ScalarRepresentation rep,
                          int comp = 0);

  /** Constructor from an integer, used for compatibility purposes, allowing
   * zero to be cast to the default mode state */
  MultiChannelDisplayMode(int value);

  /** Initialize for RGB mode */
  static MultiChannelDisplayMode DefaultForRGB();

  /** Save to registry */
  void Save(Registry &reg) const;

  /** Restore from registry */
  static MultiChannelDisplayMode Load(Registry &reg);

  static RegistryEnumMap<ScalarRepresentation> &GetScalarRepNames();

  /** Get a hash value for this struct - for ordering purposes */
  int GetHashValue() const;

  /**
   * Whether the mode is a single-component mode: i.e., non-RGB and the
   * ScalarRepresentation is single component
   */
  bool IsSingleComponent();

  /** Comparison operators */
  bool operator == (const MultiChannelDisplayMode &mode) const
    { return GetHashValue() == mode.GetHashValue(); }

  bool operator != (const MultiChannelDisplayMode &mode) const
    { return GetHashValue() != mode.GetHashValue(); }
};

bool operator < (const MultiChannelDisplayMode &a, const MultiChannelDisplayMode &b);


#endif // MULTICHANNELDISPLAYMODE_H
