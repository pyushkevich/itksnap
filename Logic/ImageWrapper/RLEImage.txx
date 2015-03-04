#ifndef RLEImage_txx
#define RLEImage_txx

#include "RLEImage.h"
#include <limits>

template< typename TPixel, typename RunLengthCounterType = unsigned short >
RLEImage< TPixel, RunLengthCounterType >
::RLEImage()
{
    //myBuffer managed automatically by STL
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::Allocate(bool initializePixels)
{
    myBuffer.resize(this->GetLargestPossibleRegion().GetSize(2));
    for (int z = 0; z < this->GetLargestPossibleRegion().GetSize(2); z++)
    {
        myBuffer[z].resize(this->GetLargestPossibleRegion().GetSize(1));
        for (int y = 0; y < this->GetLargestPossibleRegion().GetSize(1); y++)
        {
            assert(this->GetLargestPossibleRegion().GetSize(0) <= std::numeric_limits<RunLengthCounterType>::max());
            //since we have to initialize pixel values to something, we initialize to 0
            myBuffer[z][y].push_back(RLSegment(this->GetLargestPossibleRegion().GetSize(0), 0));
        }
    }
    this->ComputeOffsetTable(); //needed in PrintSelf
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::Initialize()
{
    //
    // We don't modify ourselves because the "ReleaseData" methods depend upon
    // no modification when initialized.
    //

    // Call the superclass which should initialize the BufferedRegion ivar.
    Superclass::Initialize();

    myBuffer.clear();
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::FillBuffer(const TPixel & value)
{
    assert(!myBuffer.empty());
    #pragma omp parallel for
    for (int z = 0; z < myBuffer.size(); z++)
        for (int y = 0; y < this->GetLargestPossibleRegion().GetSize(1); y++)
        {
            myBuffer[z][y].resize(1);
            myBuffer[z][y][0] = RLSegment(this->GetLargestPossibleRegion().GetSize(0), value);
        }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::SetPixelContainer(PixelContainer *container)
{
    if (m_Buffer != container)
    {
        m_Buffer = container;
        this->Modified();
    }
}

//----------------------------------------------------------------------------
//template< typename TPixel, typename RunLengthCounterType = unsigned short >
//void
//RLEImage< TPixel, RunLengthCounterType >
//::Graft(const DataObject *data)
//{
//    // call the superclass' implementation
//    Superclass::Graft(data);
//
//    if (data)
//    {
//        // Attempt to cast data to an RLEImage
//        const Self * const imgData = dynamic_cast< const Self * >(data);
//
//        if (imgData)
//        {
//            // Now copy anything remaining that is needed
//            this->SetPixelContainer(const_cast< PixelContainer * >
//                (imgData->GetPixelContainer()));
//        }
//        else
//        {
//            // pointer could not be cast back down
//            itkExceptionMacro(<< "itk::RLEImage::Graft() cannot cast "
//                << typeid(data).name() << " to "
//                << typeid(const Self *).name());
//        }
//    }
//}

//----------------------------------------------------------------------------
template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::ComputeIndexToPhysicalPointMatrices()
{
    this->Superclass::ComputeIndexToPhysicalPointMatrices();
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
unsigned int
RLEImage< TPixel, RunLengthCounterType >
::GetNumberOfComponentsPerPixel() const
{
    // use the GetLength() method which works with variable length arrays,
    // to make it work with as much pixel types as possible
    PixelType p;
    return itk::NumericTraits< PixelType >::GetLength(p);
}

/**
*
*/
template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
    Superclass::PrintSelf(os, indent);
    itk::SizeValueType c = 0;
    for (int z = 0; z < myBuffer.size(); z++)
        for (int y = 0; y < this->GetLargestPossibleRegion().GetSize(1); y++)
            c+=myBuffer[z][y].size();
    double cr = double(c*(sizeof(PixelType) + sizeof(RunLengthCounterType)))
        / (this->GetOffsetTable()[3] * sizeof(PixelType));
    os << indent << "RLEImage compressed pixel count: " << c << std::endl;
    int prec = os.precision(2);
    os << indent << "Compression ratio: "<< cr << std::endl;
    os.precision(prec);

    // m_Origin and m_Spacing are printed in the Superclass
}

#endif RLEImage_txx