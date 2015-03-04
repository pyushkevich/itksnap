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
::Allocate()
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
RLEImage< TPixel, RunLengthCounterType >::
SetPixel(const IndexType & index, const TPixel & value)
{
    RLLine & line = myBuffer[index[2]][index[1]];
    RunLengthCounterType t = 0;
    for (int x = 0; x < line.size(); x++)
    {
        t += line[x].first;
        if (t > index[0])
        {
            if (line[x].second == value) //already correct value
                return;
            else if (line[x].first == 1) //single pixel block
                line[x].second = value;
            else if (t == index[0] && x < line.size() - 1 && line[x + 1].second == value)
            {
                //shift this pixel to next block
                line[x].first--;
                line[x + 1].first++;
            }
            else if (t == index[0] + 1) //insert after
            {
                line[x].first--;
                line.insert(line.begin() + x + 1, RLSegment(1, value));
            }
            else if (t == index[0] + line[x].first) //insert before
            {
                line[x].first--;
                line.insert(line.begin() + x, RLSegment(1, value));
            }
            else //general case: split a block into 3 blocks
            {
                //first take care of values
                line.insert(line.begin() + x + 1, 2, RLSegment(1, value));
                line[x + 2].second = line[x].second;

                //now take care of counts
                line[x].first += index[0] - t;
                line[x + 2].first = t - index[0] - 1;
            }
            return;
        }
    }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
const TPixel &
RLEImage< TPixel, RunLengthCounterType >::
GetPixel(const IndexType & index) const
{
    RLLine & line = myBuffer[index[2]][index[1]];
    RunLengthCounterType t = 0;
    for (int x = 0; x < line.size(); x++)
    {
        t += line[x].first;
        if (t > index[0])
            return line[x].second;
    }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
TPixel &
RLEImage< TPixel, RunLengthCounterType >::
GetPixel(const IndexType & index)
{
    RLLine & line = myBuffer[index[2]][index[1]];
    RunLengthCounterType t = 0;
    for (int x = 0; x < line.size(); x++)
    {
        t += line[x].first;
        if (t > index[0])
            return line[x].second;
    }
}

//template< typename TPixel, typename RunLengthCounterType = unsigned short >
//void
//RLEImage< TPixel, RunLengthCounterType >
//::SetPixelContainer(PixelContainer *container)
//{
//    if (m_Buffer != container)
//    {
//        m_Buffer = container;
//        this->Modified();
//    }
//}

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

    double cr = double(c*(sizeof(PixelType) + sizeof(RunLengthCounterType))
        + sizeof(std::vector<RLLine>) * this->GetOffsetTable()[3] / this->GetOffsetTable()[1])
        / (this->GetOffsetTable()[3] * sizeof(PixelType));

    os << indent << "RLEImage compressed pixel count: " << c << std::endl;
    int prec = os.precision(3);
    os << indent << "Compressed size in relation to original size: "<< cr*100 <<"%" << std::endl;
    os.precision(prec);

    // m_Origin and m_Spacing are printed in the Superclass
}

#endif RLEImage_txx