#ifndef VECTORTOSCALARIMAGEACCESSOR_H
#define VECTORTOSCALARIMAGEACCESSOR_H

#include "itkDefaultVectorPixelAccessor.h"

namespace itk
{
template <class TImage, class TAccessor> class ImageAdaptor;
template <class TPixel, unsigned int Vdim> class VectorImage;
}

/**
 * An accessor very similar to itk::VectorImageToImageAccessor that allows us
 * to extract certain computed quantities from the vectors, such as magnitude
 */
template <class TFunctor>
class VectorToScalarImageAccessor
      : private itk::DefaultVectorPixelAccessor< typename TFunctor::PixelType >
{
public:
  typedef itk::DefaultVectorPixelAccessor< typename TFunctor::PixelType > Superclass;
  typedef typename TFunctor::PixelType ExternalType;
  typedef typename TFunctor::PixelType InternalType;
  typedef itk::SizeValueType SizeValueType;
  typedef itk::VariableLengthVector<ExternalType> ActualPixelType;
  typedef unsigned int VectorLengthType;

  inline void Set(ActualPixelType output, const ExternalType &input) const
    { output.Fill(input); }

  inline void Set(InternalType &output, const ExternalType &input,
                  const SizeValueType offset) const
    { Set( Superclass::Get(output, offset), input); }

  inline ExternalType Get(const ActualPixelType &input) const
    { return m_Functor.Get(input); }

  inline ExternalType Get(const InternalType &input,
                          const SizeValueType offset) const
    { return Get(Superclass::Get(input, offset)); }

  void SetVectorLength(VectorLengthType l)
    {
    m_Functor.SetVectorLength(l);
    Superclass::SetVectorLength(l);
    }

protected:
  TFunctor m_Functor;
};


/**
 * This class returns the magnitude of the vector scaled by the factor sqrt(|v|)
 * The scaling is so that the gradient magnitude stays in same range as the
 * range of the components in the vector. This is so that we don't need to
 * use a separate native mapping for the derived scalar wrappers.
 */
template <class TPixel>
class VectorToScalarMagnitudeFunctor
{
public:
  typedef TPixel PixelType;
  TPixel Get(const itk::VariableLengthVector<TPixel> &input) const
  {
    return static_cast<TPixel>(input.GetNorm() * m_ScaleFactor);
  }

  void SetVectorLength(unsigned int l)
    { m_ScaleFactor = sqrt(l); }
protected:
  double m_ScaleFactor;
};

template <class TPixel>
class VectorToScalarMaxFunctor
{
public:
  typedef TPixel PixelType;
  TPixel Get(const itk::VariableLengthVector<TPixel> &input) const
  {
    TPixel mymax = input[0];
    for(int i = 1; i < input.Size(); i++)
      mymax = std::max(input[i], mymax);
    return mymax;
  }

  void SetVectorLength(unsigned int l) {}
};

template <class TPixel>
class VectorToScalarMeanFunctor
{
public:
  typedef TPixel PixelType;
  TPixel Get(const itk::VariableLengthVector<TPixel> &input) const
  {
    double mean = 0.0;
    for(int i = 1; i < input.Size(); i++)
      mean += input[i];
    return static_cast<TPixel>(mean / input.Size());
  }

  void SetVectorLength(unsigned int l) {}
};


// Typedefs for GreyType
typedef VectorToScalarMagnitudeFunctor<GreyType> GreyVectorToScalarMagnitudeFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMagnitudeFunctor>
  GreyVectorMagnitudeImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMagnitudeImageAccessor>
  GreyVectorMagnitudeImageAdaptor;

typedef VectorToScalarMaxFunctor<GreyType> GreyVectorToScalarMaxFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMaxFunctor>
  GreyVectorMaxImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMaxImageAccessor>
  GreyVectorMaxImageAdaptor;

typedef VectorToScalarMeanFunctor<GreyType> GreyVectorToScalarMeanFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMeanFunctor>
  GreyVectorMeanImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMeanImageAccessor>
  GreyVectorMeanImageAdaptor;



#endif // VECTORTOSCALARIMAGEACCESSOR_H
