#ifndef MOMENTTEXTURES_H
#define MOMENTTEXTURES_H

#include "SNAPCommon.h"
#include "itkImageToImageFilter.h"

// Forward declarations
namespace itk
{
template <class TPixel, unsigned int VDim> class Image;
template <class TImage> class ImageRegionIteratorWithIndex;
}

namespace bilwaj {

template <class TInputImage, class TOutputImage>
class MomentTextureFilter
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  typedef MomentTextureFilter<TInputImage, TOutputImage> Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  itkTypeMacro(ImageToImageFilter, ImageSource)

  itkNewMacro(Self)

  itkStaticConstMacro(ImageDimension, unsigned int, TInputImage::ImageDimension);

  typedef TInputImage                                  InputImageType;
  typedef typename InputImageType::RegionType          RegionType;
  typedef typename InputImageType::SizeType            SizeType;
  typedef typename InputImageType::PixelType           InputPixelType;
  typedef TOutputImage                                 OutputImageType;
  typedef typename OutputImageType::PixelType          OutputPixelType;
  typedef typename OutputImageType::InternalPixelType  OutputComponentType;

  /** Set the radius for moment computation */
  itkSetMacro(Radius, SizeType)
  itkGetMacro(Radius, SizeType)

  /** Set the highest order of the moments */
  itkSetMacro(HighestDegree, unsigned int)
  itkGetMacro(HighestDegree, unsigned int)

protected:

  MomentTextureFilter() : m_HighestDegree(2) { m_Radius.Fill(1); }
  ~MomentTextureFilter() {}

  virtual void ThreadedGenerateData(const RegionType & outputRegionForThread,
                                    itk::ThreadIdType threadId);

  virtual void UpdateOutputInformation();

  // Highest degree for which to generate the textures
  unsigned int m_HighestDegree;

  // Radius of the neighborhood for texture generation
  SizeType m_Radius;

private:

  MomentTextureFilter(const Self &); //purposely not implemented
  void operator=(const Self &);     //purposely not implemented



};

/*
typedef short datatype;
typedef itk::Image<datatype, 3> ImageType;

void MomentTexture(SmartPtr<ImageType> &Inp,
                   SmartPtr<ImageType> &MomentTexture,
                   int degree, int Ext=1);
*/
}

#endif // MOMENTTEXTURES_H
