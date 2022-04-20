#ifndef itkComputeInterpolation_txx
#define itkComputeInterpolation_txx

#include "itkObjectFactory.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include <math.h>

namespace itk
{

template< class TInputImage, class TOutputImage>
ComputeInterpolation<TInputImage,TOutputImage>
::ComputeInterpolation()
{
    this->SetNumberOfRequiredInputs(2);
    this->SetNumberOfRequiredOutputs(2);

    this->SetNthOutput( 0, this->MakeOutput(0) );
    this->SetNthOutput( 1, this->MakeOutput(1) );

}

template< class TInputImage, class TOutputImage>
void
ComputeInterpolation<TInputImage,TOutputImage>
::SetInputImage1(const TInputImage* image)
{
    this->SetNthInput(0, const_cast<TInputImage*>(image));
}

template< class TInputImage, class TOutputImage>
void
ComputeInterpolation<TInputImage,TOutputImage>
::SetInputImage2(const TInputImage* mask)
{
    this->SetNthInput(1, const_cast<TInputImage*>(mask));
}

template< class TInputImage, class TOutputImage>
typename TInputImage::ConstPointer ComputeInterpolation<TInputImage,TOutputImage>::GetInputImage1()
{
    return static_cast< const TInputImage * >
            ( this->ProcessObject::GetInput(0) );
}

template< class TInputImage, class TOutputImage>
typename TInputImage::ConstPointer ComputeInterpolation<TInputImage,TOutputImage>::GetInputImage2()
{
    return static_cast< const TInputImage * >
            ( this->ProcessObject::GetInput(1) );
}

template< class TInputImage, class TOutputImage>
TOutputImage* ComputeInterpolation<TInputImage, TOutputImage>::GetInterpolation()
{
    return dynamic_cast< TOutputImage * >(this->ProcessObject::GetOutput(0) );
}

template< class TInputImage, class TOutputImage>
TInputImage* ComputeInterpolation<TInputImage, TOutputImage>::GetProbabilityMap()
{
    return dynamic_cast< TInputImage * >(this->ProcessObject::GetOutput(1) );
}

template< typename TInputImage, typename TOutputImage >
DataObject::Pointer ComputeInterpolation<TInputImage, TOutputImage>::MakeOutput(unsigned int idx)
{
    DataObject::Pointer output;

    switch ( idx )
    {
    case 0:
        output = ( TOutputImage::New() ).GetPointer();
        break;
    case 1:
        output = ( TInputImage::New() ).GetPointer();
        break;
    default:
        std::cerr << "No output " << idx << std::endl;
        output = NULL;
        break;
    }
    return output.GetPointer();
}

template< class TInputImage, class TOutputImage>
void
ComputeInterpolation<TInputImage,TOutputImage>
::GenerateData()
{

    double t2 = 1 - t1;

    typename TInputImage::ConstPointer inputA = this->GetInputImage1();
    typename TInputImage::ConstPointer inputB = this->GetInputImage2();

    // Setup output 1
    typename TOutputImage::Pointer interpolation = this->GetInterpolation();
    interpolation->SetBufferedRegion(interpolation->GetRequestedRegion());
    interpolation->Allocate();

    // Setup output 2
    typename TInputImage::Pointer probabilitymap= this->GetProbabilityMap();
    probabilitymap->SetBufferedRegion(probabilitymap->GetRequestedRegion());
    probabilitymap->Allocate();

    // Interpolation

    typename MultiplyImageFilterType::Pointer Multiply_A = MultiplyImageFilterType::New();
    Multiply_A->SetInput(inputA);
    Multiply_A->SetConstant(t2);
    Multiply_A->Update();

    typename MultiplyImageFilterType::Pointer Multiply_B = MultiplyImageFilterType::New();
    Multiply_B->SetInput(inputB);
    Multiply_B->SetConstant(t1);
    Multiply_B->Update();

    typename AddImageFilterType::Pointer AddDistanceMaps = AddImageFilterType::New() ;
    AddDistanceMaps->SetInput1(Multiply_A->GetOutput());
    AddDistanceMaps->SetInput2(Multiply_B->GetOutput());
    AddDistanceMaps->Update();

    typename BinaryThresholdImageFilterType::Pointer ThresholdDistanceMap = BinaryThresholdImageFilterType::New();
    ThresholdDistanceMap->SetInput(AddDistanceMaps->GetOutput());
    ThresholdDistanceMap->SetLowerThreshold(0);
    ThresholdDistanceMap->SetUpperThreshold(10000); //Very large value ~ infinity
    ThresholdDistanceMap->SetInsideValue(1);
    ThresholdDistanceMap->SetOutsideValue(0);
    ThresholdDistanceMap->Update();

    ImageAlgorithm::Copy(AddDistanceMaps->GetOutput(), probabilitymap.GetPointer(), probabilitymap->GetRequestedRegion(),
                         probabilitymap->GetRequestedRegion() );

    /** Compute error function of the interpolated distance map */
    itk::ImageRegionConstIterator<TInputImage> It(AddDistanceMaps->GetOutput(),AddDistanceMaps->GetOutput()->GetLargestPossibleRegion());
    It.GoToBegin();
    while (!It.IsAtEnd())
    {

        double x = It.Get();
        double x_erf = erf(x/5);
        double x_norm = (x_erf + 1)/2;
        probabilitymap->SetPixel(It.GetIndex(), x_norm);
        ++It;
    }

    this->GetInterpolation()->Graft(ThresholdDistanceMap->GetOutput());

}

}// end namespace

#endif
