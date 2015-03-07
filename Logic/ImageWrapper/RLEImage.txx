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
            myBuffer[z][y].push_back(RLSegment(RunLengthCounterType(this->GetLargestPossibleRegion().GetSize(0)), TPixel(0)));
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
            myBuffer[z][y][0] = RLSegment(RunLengthCounterType(this->GetLargestPossibleRegion().GetSize(0)), value);
        }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >::
SetPixel(const IndexType & index, const TPixel & value)
{
    RLLine & line = myBuffer[index[2]][index[1]];
    RunLengthCounterType t = 0;
    for (itk::SizeValueType x = 0; x < line.size(); x++)
    {
        t += line[x].first;
        if (t > index[0])
        {
            if (line[x].second == value) //already correct value
                return;
            else if (line[x].first == 1) //single pixel segment
                line[x].second = value;
            else if (t == index[0] && x < line.size() - 1 && line[x + 1].second == value)
            {
                //shift this pixel to next segment
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
            else //general case: split a segment into 3 segments
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
    throw itk::ExceptionObject(__FILE__, __LINE__, "Reached past the end of Run-Length line!", __FUNCTION__);
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void
RLEImage< TPixel, RunLengthCounterType >::
fromITKImage(typename itk::Image<TPixel, 3>::Pointer image)
{
    this->CopyInformation(image);
    this->SetRegions(image->GetLargestPossibleRegion());

    itk::Size<3> size = image->GetLargestPossibleRegion().GetSize();
    myBuffer.clear(); //in case it wasn't already
    myBuffer.resize(size[2]);
    for (itk::SizeValueType z = 0; z < size[2]; z++)
        myBuffer[z].resize(size[1]);

    RLLine temp;
    temp.reserve(size[0]); //pessimistically preallocate buffer, otherwise reallocations can occur
    itk::Index<3> ind;
    ind[0] = 0;
    for (SizeValueType z = 0; z < size[2]; z++)
    {
        ind[2] = z;
        for (SizeValueType y = 0; y < size[1]; y++)
        {
            ind[1] = y;
            SizeValueType x = 0;       
            TPixel * p = image->GetBufferPointer();
            p+=image->ComputeOffset(ind);
            temp.clear();
            while (x < size[0])
            {
                RLSegment s(0, *p);
                while (x < size[0] && *p == s.second)
                {
                    x++;
                    s.first++;
                    p++;
                }
                temp.push_back(s);
            }
            myBuffer[z][y] = temp;
        }
    }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
typename itk::Image<TPixel, 3>::Pointer
RLEImage< TPixel, RunLengthCounterType >::
toITKImage() const
{
    itk::Image<TPixel, 3>::Pointer out = itk::Image<TPixel, 3>::New();
    out->CopyInformation(this);
    out->SetRegions(this->GetLargestPossibleRegion());
    out->Allocate(false);

    itk::Size<3> size;
    size[2] = myBuffer.size(); //this->GetLargestPossibleRegion().GetSize(2);
    size[1] = myBuffer[0].size(); //this->GetLargestPossibleRegion().GetSize(1);
    //size[0] = 0;
    //for (int x = 0; x < myBuffer[0][0].size(); x++)
    //    size[0] += myBuffer[0][0][x].first;
    size[0] = this->GetLargestPossibleRegion().GetSize(0);

    TPixel * p = out->GetBufferPointer();
    itk::Index<3> ind;
    ind[0] = 0;
    for (SizeValueType z = 0; z < size[2]; z++)
    {
        ind[2] = z;
        for (SizeValueType y = 0; y < size[1]; y++)
        {
            ind[1] = y;
            SizeValueType x = 0;
            SizeValueType offset = out->ComputeOffset(ind);
            uncompressLine(myBuffer[z][y], p + offset);
        }
    }
    return out;
}


//int sliceIndex, axis;
//Seg3DImageType::RegionType reg;
//
//void cropRLI(RLImage image, short *outSlice)
//{
//    itk::Size<3> size;
//    size[2] = image.size();
//    size[1] = image[0].size();
//    size[0] = 0;
//    for (int x = 0; x < image[0][0].size(); x++)
//        size[0] += image[0][0][x].first;
//
//    if (axis == 2) //slicing along z
//        for (int y = 0; y < size[1]; y++)
//            uncompressLine(image[sliceIndex][y], outSlice + y*size[0]);
//    else if (axis == 1) //slicing along y
//        for (int z = 0; z < size[2]; z++)
//            uncompressLine(image[z][sliceIndex], outSlice + z*size[0]);
//    else //slicing along x, the low-preformance case
//        for (int z = 0; z < size[2]; z++)
//            for (int y = 0; y < size[1]; y++)
//            {
//                int t = 0;
//                for (int x = 0; x < image[z][y].size(); x++)
//                {
//                    t += image[z][y][x].first;
//                    if (t > sliceIndex)
//                    {
//                        *(outSlice++) = image[z][y][x].second;
//                        break;
//                    }
//                }
//            }
//}

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

#endif //RLEImage_txx