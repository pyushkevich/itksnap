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
      : private itk::DefaultVectorPixelAccessor< typename TFunctor::InputPixelType >
{
public:
  typedef itk::DefaultVectorPixelAccessor< typename TFunctor::InputPixelType > Superclass;
  typedef typename TFunctor::OutputPixelType ExternalType;
  typedef typename TFunctor::InputPixelType InternalType;
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

  /**
   * Set the linear mapping from the vector image internal values to the
   * native data type. The mapping is native = internal * scale + shift
   */
  void SetSourceNativeMapping(double scale, double shift)
  {
    m_Functor.SetSourceNativeMapping(scale, shift);
  }

protected:
  TFunctor m_Functor;
};

/**
 * This is the parent class for the functors below, encompassing shared
 * functionality
 */
class AbstractVectorToDerivedQuantityFunctor
{
public:

  AbstractVectorToDerivedQuantityFunctor()
  {
    m_Length = 1;
    m_Scale = 1;
    m_Shift = 0;
    this->ParametersUpdated();
  }

  virtual ~AbstractVectorToDerivedQuantityFunctor() {}

  virtual void SetSourceNativeMapping(double scale, double shift)
  {
    m_Shift = shift;
    m_Scale = scale;
    this->ParametersUpdated();
  }

  virtual void SetVectorLength(unsigned int l)
  {
    m_Length = l;
    this->ParametersUpdated();
  }

  virtual void ParametersUpdated() {};

protected:

  // Mapping from internal to native in the wrapped image
  double m_Shift, m_Scale;
  unsigned int m_Length;
};



/**
 * This class returns the magnitude of the vector. Since the vector is stored
 * internally as a shifted and scaled version of the native intensity value,
 * to compute magnitude, we first need to map the input values back to the
 * native value, take magnitude, and map back to the stored integral value
 *
 * TODO: magnitude adapters should be treated as float images, not short,
 * which will allow us to avoid this mess.
 */
template <class TInputPixel, class TOutputPixel>
class VectorToScalarMagnitudeFunctor : public AbstractVectorToDerivedQuantityFunctor
{
public:
  typedef TInputPixel InputPixelType;
  typedef TOutputPixel OutputPixelType;
  OutputPixelType Get(const itk::VariableLengthVector<InputPixelType> &input) const
  {
    double sumT2 = 0.0, sumT = 0.0;
    for(int i = 0; i < input.Size(); i++)
      {
      double t = input[i];
      sumT2 += t * t;
      sumT += t;
      }

    double norm_raw_out =
        sqrt(m_CoeffT2 * sumT2 + m_CoeffT1 * sumT + m_CoeffT0);

    return static_cast<OutputPixelType>(norm_raw_out);
  }

  virtual void ParametersUpdated()
  {
    m_CoeffT2 = (this->m_Scale * this->m_Scale);
    m_CoeffT1 = (2 * this->m_Scale * this->m_Shift);
    m_CoeffT0 = (this->m_Shift * this->m_Shift) * this->m_Length;
  }

protected:

  // Used internally to compute norm with fewest operations
  double m_CoeffT2, m_CoeffT1, m_CoeffT0;
};

template <class TInputPixel, class TOutputPixel>
class VectorToScalarMaxFunctor : public AbstractVectorToDerivedQuantityFunctor
{
public:
  typedef TInputPixel InputPixelType;
  typedef TOutputPixel OutputPixelType;
  OutputPixelType Get(const itk::VariableLengthVector<InputPixelType> &input) const
  {
    InputPixelType mymax = input[0];
    for(int i = 1; i < input.Size(); i++)
      mymax = std::max(input[i], mymax);
    return static_cast<OutputPixelType>(mymax * this->m_Scale + this->m_Shift);
  }
};

template <class TInputPixel, class TOutputPixel>
class VectorToScalarMeanFunctor : public AbstractVectorToDerivedQuantityFunctor
{
public:
  typedef TInputPixel InputPixelType;
  typedef TOutputPixel OutputPixelType;

  OutputPixelType Get(const itk::VariableLengthVector<InputPixelType> &input) const
  {
    double mean = 0.0;
    for(int i = 0; i < input.Size(); i++)
      mean += input[i];
    mean /= input.Size();
    return static_cast<OutputPixelType>(mean * this->m_Scale + this->m_Shift);
  }
};


// Typedefs for GreyType
typedef VectorToScalarMagnitudeFunctor<GreyType, float> GreyVectorToScalarMagnitudeFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMagnitudeFunctor>
  GreyVectorMagnitudeImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMagnitudeImageAccessor>
  GreyVectorMagnitudeImageAdaptor;

typedef VectorToScalarMaxFunctor<GreyType, float> GreyVectorToScalarMaxFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMaxFunctor>
  GreyVectorMaxImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMaxImageAccessor>
  GreyVectorMaxImageAdaptor;

typedef VectorToScalarMeanFunctor<GreyType,float> GreyVectorToScalarMeanFunctor;
typedef VectorToScalarImageAccessor<GreyVectorToScalarMeanFunctor>
  GreyVectorMeanImageAccessor;
typedef itk::ImageAdaptor< itk::VectorImage<GreyType, 3>, GreyVectorMeanImageAccessor>
  GreyVectorMeanImageAdaptor;



#endif // VECTORTOSCALARIMAGEACCESSOR_H
