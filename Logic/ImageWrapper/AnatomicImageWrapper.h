#ifndef ANATOMICIMAGEWRAPPER_H
#define ANATOMICIMAGEWRAPPER_H

#include <itkVectorImage.h>
#include <ImageWrapper.h>
#include <itkVectorImageToImageAdaptor.h>




class AnatomicImageWrapperBase : public ImageWrapperBase
{
public:

};

/**
 * A continuous image wrapper is for scalar and vector images that have values
 * in a continuous range, as opposed to label images, which are discrete.
 */
template <class TImage, class TBase>
class ContinuousImageWrapper
    : public ImageWrapper<TImage, TBase>
{

};

struct VectorImageRGBDisplayMode
{
public:
  bool UsePalette;
  unsigned int ActiveChannel;
  Vector3ui RGBChannels;
};

template <class TPixel>
class AnatomicImageWrapper
    : public ImageWrapper<itk::VectorImage<TPixel, 3>, AnatomicImageWrapperBase>
{
public:
  AnatomicImageWrapper();

  virtual DisplaySlicePointer GetDisplaySlice(unsigned int dim);

  typedef itk::VectorImageToImageAdaptor<TPixel, 3> ComponentAdaptor;
  typedef ImageWrapper<ComponentAdaptor, ImageWrapperBase> ComponentWrapper;


protected:
  VectorImageRGBDisplayMode m_DisplayMode;

};

#endif // ANATOMICIMAGEWRAPPER_H
