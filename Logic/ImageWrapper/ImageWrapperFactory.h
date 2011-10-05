#ifndef IMAGEWRAPPERFACTORY_H
#define IMAGEWRAPPERFACTORY_H

#include "ImageWrapperBase.h"
#include "itkImageBase.h"

class ImageWrapperFactory
{
public:
  typedef itk::ImageBase<3> ImageBaseType;


  static GreyImageWrapperBase* NewGreyImageWrapper(ImageBaseType *image);

  static RGBImageWrapperBase* NewRGBImageWrapper(ImageBaseType *image);

};

#endif // IMAGEWRAPPERFACTORY_H
