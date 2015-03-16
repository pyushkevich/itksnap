#ifndef RLEImage_txx
#define RLEImage_txx

#include "RLEImage.h"
#include <limits>

template< typename TPixel, typename RunLengthCounterType = unsigned short >
RLEImage< TPixel, RunLengthCounterType >
::RLEImage()
{
    m_OnTheFlyCleanup = true;
    //myBuffer managed automatically by STL
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void RLEImage< TPixel, RunLengthCounterType >::Allocate()
{
    myBuffer.resize(this->GetLargestPossibleRegion().GetSize(2));
#pragma omp parallel for
    for (int z = 0; z < this->GetLargestPossibleRegion().GetSize(2); z++)
    {
        myBuffer[z].resize(this->GetLargestPossibleRegion().GetSize(1));
        for (int y = 0; y < this->GetLargestPossibleRegion().GetSize(1); y++)
        {
            assert(this->GetLargestPossibleRegion().GetSize(0) <= std::numeric_limits<RunLengthCounterType>::max());
            //since we have to initialize pixel values to something, we initialize using default constructor
            myBuffer[z][y].push_back(RLSegment(RunLengthCounterType(this->GetLargestPossibleRegion().GetSize(0)), TPixel()));
        }
    }
    this->ComputeOffsetTable(); //needed in PrintSelf
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void RLEImage< TPixel, RunLengthCounterType >::Initialize()
{
    //
    // We don't modify ourselves because the "ReleaseData" methods depend upon
    // no modification when initialized.
    //

    // Call the superclass which should initialize the BufferedRegion ivar.
    Superclass::Initialize();
    m_OnTheFlyCleanup = true;
    myBuffer.clear();
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void RLEImage< TPixel, RunLengthCounterType >
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
void RLEImage< TPixel, RunLengthCounterType >::CleanUpLine(RLLine & line) const
{
    RunLengthCounterType x = 0;
    RLLine out;
    out.reserve(this->GetLargestPossibleRegion().GetSize(0));
    do
    {
        out.push_back(line[x]);
        while (++x < line.size() && line[x].second == line[x - 1].second)
            out.back().first += line[x].first;
    } while (x < line.size());
    out.swap(line);
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void RLEImage< TPixel, RunLengthCounterType >::CleanUp() const
{
    assert(!myBuffer.empty());
    if (this->GetLargestPossibleRegion().GetSize(0) == 0)
        return;
#pragma omp parallel for
    for (RunLengthCounterType z = 0; z < myBuffer.size(); z++)
        for (RunLengthCounterType y = 0; y < myBuffer[0].size(); y++)
            CleanUpLine(myBuffer[z][y]);
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
int RLEImage< TPixel, RunLengthCounterType >::
SetPixel(RLLine & line, IndexValueType & segmentRemainder, IndexValueType & realIndex, const TPixel & value)
{
    if (line[realIndex].second == value) //already correct value
        return 0;
    else if (line[realIndex].first == 1) //single pixel segment
    {
        line[realIndex].second = value;
        if (m_OnTheFlyCleanup)//now see if we can merge it into adjacent segments
        {
            if (realIndex>0 && realIndex < line.size() - 1 &&
                line[realIndex + 1].second == value && line[realIndex - 1].second == value)
            {
                //merge these 3 segments
                line[realIndex - 1].first += 1 + line[realIndex + 1].first;
                segmentRemainder += line[realIndex + 1].first;
                line.erase(line.begin() + realIndex, line.begin() + realIndex + 2);
                realIndex--;
                return -2;
            }
            if (realIndex>0 && line[realIndex - 1].second == value)
            {
                //merge into previous
                line[realIndex - 1].first++;
                line.erase(line.begin() + realIndex);
                realIndex--; assert(segmentRemainder == 1);
                return -1;
            }
            else if (realIndex < line.size() - 1 && line[realIndex + 1].second == value)
            {
                //merge into next
                segmentRemainder = ++(line[realIndex + 1].first);
                line.erase(line.begin() + realIndex);
                return -1;
            }
        }
        return 0;
    }
    else if (segmentRemainder==1 && realIndex < line.size() - 1 && line[realIndex + 1].second == value)
    {
        //shift this pixel to next segment
        line[realIndex].first--;
        segmentRemainder = ++(line[realIndex + 1].first);
        realIndex++;
        return 0;
    }
    else if (realIndex>0 && segmentRemainder == line[realIndex].first && line[realIndex - 1].second == value)
    {
        //shift this pixel to previous segment
        line[realIndex].first--;
        line[realIndex - 1].first++;
        realIndex--;
        segmentRemainder = 1;
        return 0;
    }
    else if (segmentRemainder == 1) //insert after
    {
        line[realIndex].first--;
        line.insert(line.begin() + realIndex + 1, RLSegment(1, value));
        realIndex++;
        return +1;
    }
    else if (segmentRemainder == line[realIndex].first) //insert before
    {
        line[realIndex].first--;
        line.insert(line.begin() + realIndex, RLSegment(1, value));
        segmentRemainder = 1;
        return +1;
    }
    else //general case: split a segment into 3 segments
    {
        //first take care of values
        line.insert(line.begin() + realIndex + 1, 2, RLSegment(1, value));
        line[realIndex + 2].second = line[realIndex].second;

        //now take care of counts
        line[realIndex].first += line[realIndex].first-segmentRemainder;
        line[realIndex + 2].first = segmentRemainder - 1;
        realIndex++;
        segmentRemainder = 1;
        return +2;
    }
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
void RLEImage< TPixel, RunLengthCounterType >::
SetPixel(const IndexType & index, const TPixel & value)
{
    RLLine & line = myBuffer[index[2]][index[1]];
    RunLengthCounterType t = 0;
    for (itk::SizeValueType x = 0; x < line.size(); x++)
    {
        t += line[x].first;
        if (t > index[0])
        {
            t -= index[0]; //we need to supply a reference
            SetPixel(line, t, x, value);
            return;
        }
    }
    throw itk::ExceptionObject(__FILE__, __LINE__, "Reached past the end of Run-Length line!", __FUNCTION__);
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
const TPixel & RLEImage< TPixel, RunLengthCounterType >::
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
    throw itk::ExceptionObject(__FILE__, __LINE__, "Reached past the end of Run-Length line!", __FUNCTION__);
}

template< typename TPixel, typename RunLengthCounterType = unsigned short >
TPixel & RLEImage< TPixel, RunLengthCounterType >::
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
void RLEImage< TPixel, RunLengthCounterType >::
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
#pragma omp parallel for
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
RLEImage< TPixel, RunLengthCounterType >::toITKImage() const
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
#pragma omp parallel for
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
unsigned int RLEImage< TPixel, RunLengthCounterType >
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
void RLEImage< TPixel, RunLengthCounterType >
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

    os << indent << "OnTheFlyCleanup: " << (m_OnTheFlyCleanup ? "On" : "Off") << std::endl;
    os << indent << "RLEImage compressed pixel count: " << c << std::endl;
    int prec = os.precision(3);
    os << indent << "Compressed size in relation to original size: "<< cr*100 <<"%" << std::endl;
    os.precision(prec);

    // m_Origin and m_Spacing are printed in the Superclass
}

#endif //RLEImage_txx