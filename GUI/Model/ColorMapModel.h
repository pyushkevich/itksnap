#ifndef COLORMAPMODEL_H
#define COLORMAPMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "GreyImageWrapper.h"

class ColorMapLayerProperties
{
public:

  irisGetSetMacro(ObserverTag, unsigned long)

protected:

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

class ColorMapModel : public AbstractLayerAssociatedModel<
    ColorMapLayerProperties, GreyImageWrapperBase>
{
public:
  irisITKObjectMacro(ColorMapModel, AbstractLayerAssociatedModel)

protected:

  ColorMapModel();
};

#endif // COLORMAPMODEL_H
