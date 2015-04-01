#ifndef MOMENTTEXTURES_H
#define MOMENTTEXTURES_H

#include "SNAPCommon.h"

// Forward declarations
namespace itk
{
template <class TPixel, unsigned int VDim> class Image;
template <class TImage> class ImageRegionIteratorWithIndex;
}

namespace bilwaj {

typedef short datatype;
typedef itk::Image<datatype, 3> ImageType;
typedef itk::ImageRegionIteratorWithIndex<ImageType> IteratorType;

void MomentTexture(SmartPtr<ImageType> &Inp,
                   SmartPtr<ImageType> &MomentTexture,
                   int degree, int Ext=1);

}

#endif // MOMENTTEXTURES_H
