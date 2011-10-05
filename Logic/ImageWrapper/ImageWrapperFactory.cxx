#include "ImageWrapperFactory.h"
#include "GreyImageWrapper.h"
#include "RGBImageWrapper.h"

ImageWrapperFactory::ImageWrapperFactory()
{
}

template <class TPixel>
GreyImageWrapperBase *
make_grey(ImageBaseType *base)
{
  itk::Image<TPixel> *image = dynamic_cast<itk::Image<TPixel> *>(base);
  if(image)
    {
    GreyImageWrapper<TPixel> *wrapper = new GreyImageWrapper<TPixel>;
    wrapper->SetImage(image);
    return wrapper;
    }
  return NULL;
}

template <class TComponent>
RGBImageWrapperBase *
make_rgb(ImageBaseType *base)
{
  itk::Image<TPixel> *image = dynamic_cast<itk::Image<TPixel> *>(base);
  if(image)
    {
    RGBImageWrapper<TPixel> *wrapper = new RGBImageWrapper<TPixel>;
    wrapper->SetImage(image);
    return wrapper;
    }
  return NULL;
}


GreyImageWrapperBase *
ImageWrapperFactory
::NewGreyImageWrapper(ImageBaseType *image)
{
  GreyImageWrapperBase *w = NULL;

  w = make_grey<unsigned char>(image);
  if(w)
    return w;

  w = make_grey<char>(image);
  if(w)
    return w;

  w = make_grey<unsigned short>(image);
  if(w)
    return w;

  w = make_grey<short>(image);
  if(w)
    return w;

  w = make_grey<unsigned int>(image);
  if(w)
    return w;

  w = make_grey<int>(image);
  if(w)
    return w;

  w = make_grey<unsigned long>(image);
  if(w)
    return w;

  w = make_grey<long>(image);
  if(w)
    return w;

  w = make_grey<float>(image);
  if(w)
    return w;

  w = make_grey<double>(image);
  if(w)
    return w;

  return NULL;
}

RGBImageWrapperBase *
ImageWrapperFactory
::NewRGBImageWrapper(ImageBaseType *image)
{
  RGBImageWrapperBase *w = NULL;

  w = make_rgb<unsigned char>(image);
  if(w)
    return w;

  w = make_rgb<char>(image);
  if(w)
    return w;

  w = make_rgb<unsigned short>(image);
  if(w)
    return w;

  w = make_rgb<short>(image);
  if(w)
    return w;

  w = make_rgb<unsigned int>(image);
  if(w)
    return w;

  w = make_rgb<int>(image);
  if(w)
    return w;

  w = make_rgb<unsigned long>(image);
  if(w)
    return w;

  w = make_rgb<long>(image);
  if(w)
    return w;

  w = make_rgb<float>(image);
  if(w)
    return w;

  w = make_rgb<double>(image);
  if(w)
    return w;

  return NULL;
}
