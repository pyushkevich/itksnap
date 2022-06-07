//#ifndef ITKBWAANDRFINTERPOLATION_HXX
//#define ITKBWAANDRFINTERPOLATION_HXX

#include <math.h>
#include <itkSize.h>
#include <iostream>
#include "itkObjectFactory.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkBWAandRFinterpolation.h"
#include "ImageCollectionToImageFilter.h"

namespace itk
{

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::CombineBWAandRFFilter ()
    : m_Label( 0 ),
      m_UserAxis( -1 ),
      m_LabeledSlices( TLabelImage::ImageDimension ) // initialize with empty sets
{
    this->SetNumberOfRequiredInputs(2);
    this->SetNumberOfRequiredOutputs(2);

    this->SetNthOutput( 0, this->MakeOutput(0) );
    this->SetNthOutput( 1, this->MakeOutput(1) );

}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter <ImageScalarType,ImageVectorType,TLabelImage>
::AddScalarImage(const ImageScalarType *image)
{
    this->SetNthInput(0, const_cast<ImageScalarType*>(image));
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::AddVectorImage(const ImageVectorType *image)
{
    this->SetNthInput(0, const_cast<ImageVectorType*>(image));
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::SetSegmentationImage(const TLabelImage* mask)
{
    this->SetNthInput(1, const_cast<TLabelImage*>(mask));
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
typename TLabelImage::ConstPointer
CombineBWAandRFFilter<ImageScalarType,  ImageVectorType,TLabelImage>
::GetSegmentationImage()
{
    return static_cast< const TLabelImage * >
            ( this->ProcessObject::GetInput(1) );
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
TLabelImage * CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::GetInterpolation()
{
    return dynamic_cast< TLabelImage * >(this->ProcessObject::GetOutput(0) );
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
ProbabilityType * CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::GetProbabilityMap()
{
    return dynamic_cast< ProbabilityType * >(this->ProcessObject::GetOutput(1) );
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
DataObject::Pointer CombineBWAandRFFilter<ImageScalarType,  ImageVectorType,TLabelImage>
::MakeOutput(unsigned int idx)
{
    DataObject::Pointer output;

    switch ( idx )
    {
    case 0:
        output = ( TLabelImage::New() ).GetPointer();
        break;
    case 1:
        output = ( ProbabilityType::New() ).GetPointer();
        break;
    default:
        std::cerr << "No output " << idx << std::endl;
        output = NULL;
        break;
    }
    return output.GetPointer();
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
template< typename T2 >
void
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::ExpandRegion( typename T2::RegionType& region, const typename T2::IndexType& index )
{
    for ( unsigned int a = 0; a < T2::ImageDimension; ++a )
    {
        if ( region.GetIndex( a ) > index[a] )
        {
            region.SetSize( a, region.GetSize( a ) + region.GetIndex( a ) - index[a] );
            region.SetIndex( a, index[a] );
        }
        else if ( region.GetIndex( a ) + (typename T2::IndexValueType)region.GetSize( a ) <= index[a] )
        {
            region.SetSize( a, index[a] - region.GetIndex( a ) + 1 );
        }
        // else it is already within
    }
}

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter<ImageScalarType,  ImageVectorType,TLabelImage>
::DetermineSliceOrientations()
{
    m_LabeledSlices.clear();
    m_LabeledSlices.resize( TLabelImage::ImageDimension ); // initialize with empty sets
    m_BoundingBoxes.clear();

    typename TLabelImage::ConstPointer m_Input = this->GetSegmentationImage();
    typename TLabelImage::Pointer m_Output = this->GetInterpolation();

    typename TLabelImage::RegionType region = m_Output->GetRequestedRegion();
    ImageRegionConstIteratorWithIndex< TLabelImage > it( m_Input, region );
    while ( !it.IsAtEnd() )
    {
        typename TLabelImage::IndexType indPrev, indNext;
        const typename TLabelImage::IndexType ind = it.GetIndex();
        const typename TLabelImage::PixelType val = m_Input->GetPixel( ind );
        if ( val != 0 )
        {
            typename TLabelImage::RegionType boundingBox1;
            boundingBox1.SetIndex( ind );
            for ( unsigned int a = 0; a < TLabelImage::ImageDimension; ++a )
            {
                boundingBox1.SetSize( a, 1 );
            } //region of size [1,1,1]
            std::pair< typename BoundingBoxesType::iterator, bool > resBB = m_BoundingBoxes.insert( std::make_pair( val, boundingBox1 ) );
            if ( !resBB.second ) // include this index in existing BB
            {
                ExpandRegion< TLabelImage >( resBB.first->second, ind );
            }

            unsigned int cTrue = 0;
            unsigned int cAdjacent = 0;
            unsigned int axis = 0;
            for ( unsigned int a = 0; a < TLabelImage::ImageDimension; ++a )
            {
                indPrev = ind;
                indPrev[a]--;
                indNext = ind;
                indNext[a]++;

                typename TLabelImage::PixelType prev = 0;
                if ( region.IsInside( indPrev ) )
                {
                    prev = m_Input->GetPixel( indPrev );
                }
                typename TLabelImage::PixelType next = 0;
                if ( region.IsInside( indNext ) )
                {
                    next = m_Input->GetPixel( indNext );
                }
                if ( prev != val && next != val ) // && - isolated slices only, || - flat edges too SR changed this to !=val instead of zerp
                {
                    axis = a;
                    ++cTrue;
                }
                else if ( prev == val && next == val )
                {
                    ++cAdjacent;
                }
            }
            if ( cTrue == 1 && cAdjacent == TLabelImage::ImageDimension - 1 )
                // slice has empty adjacent space only along one axis
            {
                if ( m_UserAxis == -1 || m_UserAxis == int(axis) )
                 {
                    if (m_Label == 0 || m_Label == val){
                         m_LabeledSlices[axis][val].insert( ind[axis] ); // Set so doesn't take duplicate values
                    }

                }
            }
        }
        ++it;
    }

} // >::DetermineSliceOrientations

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::InterpolateLabel(typename TLabelImage::PixelType label, std::vector<int> SegIndices,typename TLabelImage::RegionType bbox )
{
    // Get input segmentation image
    typename TLabelImage::ConstPointer SegmentationImage = this->GetSegmentationImage();

    // Pointer to output image
    typename TLabelImage::Pointer m_Output = this->GetInterpolation();

    DataObject *dataobj = this->ProcessObject::GetInput(0);

    // Threshold the image so that it only contains the label of interest
    typename BinarizerType::Pointer Binarizer = BinarizerType::New();
    Binarizer->SetInput(SegmentationImage);
    Binarizer->SetLowerThreshold( label );
    Binarizer->SetUpperThreshold( label );
    Binarizer->SetInsideValue(1);
    Binarizer->SetOutsideValue(0);
    Binarizer->Update();

    // Perform contour interpolation
    typename BWAFilterType::Pointer BWAfilter = BWAFilterType::New();
    BWAfilter->SetInput(Binarizer->GetOutput());
    BWAfilter->SetIntermediateSlicesOnly(m_intermediateslices);
    BWAfilter->SetSlicingAxis(m_SlicingAxis);
    BWAfilter->SetSegmentationIndices(SegIndices);
    BWAfilter->SetBoundingBox(bbox);
    BWAfilter->Update();

    if(m_ContourInformationOnly){

        this->GetProbabilityMap()->Graft(BWAfilter->GetProbabilityMap());

        typename TLabelImage::Pointer interpolated_region = BWAfilter->GetInterpolation();

        // Interpolation made to have the same label as the interpolated label in order to paint segmentation correctly
        ImageRegionIterator< TLabelImage > itO( interpolated_region.GetPointer(),interpolated_region->GetBufferedRegion() );
        while ( !itO.IsAtEnd() )
        {
            if( itO.Get() != 0){
                itO.Set(label);
            }
            ++itO;
        }

        ImageAlgorithm::Copy< TLabelImage, TLabelImage >( interpolated_region.GetPointer(), m_Output.GetPointer(),
                                                          bbox, bbox);

    }

    if(!m_ContourInformationOnly){ // Incorporate intensity information using random forests

        typename RFLabelFilterType::Pointer generateRFlabelmap = RFLabelFilterType::New();
        generateRFlabelmap->SetInput(Binarizer->GetOutput());
        generateRFlabelmap->SetSlicingAxis(m_SlicingAxis);
        generateRFlabelmap->Update();

        typename RandomForestClassifierType::Pointer randomForestClassifier = RandomForestClassifierType::New();

        ImageScalarType *image = dynamic_cast<ImageScalarType *>(dataobj);
        if(image)
        {
            randomForestClassifier->AddScalarImage(image);
        }
        else
        {
            ImageVectorType *vecImage = dynamic_cast<ImageVectorType *>(dataobj);
            if(vecImage)
            {
                randomForestClassifier->AddVectorImage(vecImage);
            }
            else
            {
                itkAssertInDebugOrThrowInReleaseMacro(
                            "Wrong input type to ImageCollectionConstRegionIteratorWithIndex");
            }
        }

        randomForestClassifier->SetLabelMap(generateRFlabelmap->GetOutput());
        randomForestClassifier->SetBoundingBox(bbox);
        randomForestClassifier->SetIntermediateSlicesOnly(m_intermediateslices);
        randomForestClassifier->SetSegmentationIndices(SegIndices);
        randomForestClassifier->SetSlicingAxis(m_SlicingAxis);
        randomForestClassifier->Update();

        // Combine probability maps by taking the average
        typename AddImageFilterType::Pointer addProbabilityMaps = AddImageFilterType::New();
        addProbabilityMaps->SetInput1(BWAfilter->GetProbabilityMap());
        addProbabilityMaps->SetInput2(randomForestClassifier->GetOutput());

        typename MultiplyImageFilterType::Pointer scaleProbabilityMap = MultiplyImageFilterType::New();
        scaleProbabilityMap->SetInput(addProbabilityMaps->GetOutput());
        scaleProbabilityMap->SetConstant(0.5);
        scaleProbabilityMap->Update();

        //Threshold the probability map to obtain the final segmentation
        typename BinaryThresholdFilterType::Pointer thresholdProbabilityMap = BinaryThresholdFilterType::New();
        thresholdProbabilityMap->SetInput(scaleProbabilityMap->GetOutput());
        thresholdProbabilityMap->SetLowerThreshold(0.5);
        thresholdProbabilityMap->SetUpperThreshold(1);
        thresholdProbabilityMap->SetInsideValue(1);
        thresholdProbabilityMap->Update();

        //this->GetInterpolation()->Graft(thresholdProbabilityMap->GetOutput());

        this->GetProbabilityMap()->Graft(scaleProbabilityMap->GetOutput());

        typename TLabelImage::Pointer interpolated_region = thresholdProbabilityMap->GetOutput();

        // Interpolation made to have the same label as the interpolated label in order to paint segmentation correctly
        ImageRegionIterator< TLabelImage > itO( interpolated_region.GetPointer(),interpolated_region->GetBufferedRegion() );
        while ( !itO.IsAtEnd() )
        {
            if( itO.Get() != 0){
                itO.Set(label);
            }
            ++itO;
        }

        ImageAlgorithm::Copy< TLabelImage, TLabelImage >( interpolated_region.GetPointer(), m_Output.GetPointer(),
                                                          bbox, bbox);

    }

}// >::InterpolateLabel

template< class ImageScalarType, class ImageVectorType, class TLabelImage>
void
CombineBWAandRFFilter <ImageScalarType,  ImageVectorType,TLabelImage>
::GenerateData()
{

    typename TLabelImage::ConstPointer SegmentationImage = this->GetSegmentationImage();

    //DataObject *dataobj = this->ProcessObject::GetInput(0);

    typename TLabelImage::Pointer m_Output = this->GetInterpolation();
    this->AllocateOutputs();

    this->DetermineSliceOrientations();

    if ( m_BoundingBoxes.size() == 0)
    {
        ImageAlgorithm::Copy< TLabelImage, TLabelImage >( SegmentationImage.GetPointer(), m_Output.GetPointer(),
                                                          m_Output->GetRequestedRegion(), m_Output->GetRequestedRegion() );
        return; // no contours detected - no segmentations drawn
    }

    typename TLabelImage::RegionType bbox;

    // If the user entered a slicing direction, check that it is a valid axis for interpolation
    if (m_UserAxis != -1)
        if (!(m_LabeledSlices[m_UserAxis].size() >= 1)) // If it is empty
            throw itk::ExceptionObject("Invalid axis selcted for interpolation");

    // To dertemine slicing direcion,get the size of m_LabelledSlices along each axis
    bool flag_slice = 0;
    for ( unsigned axis = 0; axis < TLabelImage::ImageDimension; axis++ ) // loop through axes
    {
        if ((m_LabeledSlices[axis].size() != 0) && (flag_slice == 1))
        {
            throw itk::ExceptionObject("Detected multiple slicing directions. Please select the option to manually specify the interpolation axis");
        }

        if ((m_LabeledSlices[axis].size() != 0) && (flag_slice == 0)) //if the axis has some labelled slices then
        {
            flag_slice = 1;
            m_SlicingAxis = axis;
        }
    }

    // Interpolate along the detected slicing direction
    if (m_Label == 0){ // If all the labels need to be interpolated

        for ( typename LabeledSlicesType::iterator itS = m_LabeledSlices[m_SlicingAxis].begin();itS != m_LabeledSlices[m_SlicingAxis].end(); ++itS )
        {
            typename TLabelImage::PixelType label = itS->first;
            std::cout << "label" << label << std::endl;

            //Determine the bounding box for the label of interest
            for ( typename BoundingBoxesType::iterator iBB = m_BoundingBoxes.begin();iBB != m_BoundingBoxes.end(); ++iBB )
            {
                if (label == iBB->first)
                {
                    bbox = iBB->second;
                }
            }


            //Iterate in the detected slicing direction and include any slice containing the label of interest
            // Added this since DetermineSlicingDirection doesn't always detect the correct slices
            std::set<int> unique_slices;

            ImageRegionConstIteratorWithIndex< TLabelImage > it( SegmentationImage, SegmentationImage->GetRequestedRegion() );
            while ( !it.IsAtEnd() )
            {
                const typename TLabelImage::IndexType ind = it.GetIndex();
                const typename TLabelImage::PixelType val = SegmentationImage->GetPixel( ind );
                if (val == label)
                {
                    unique_slices.insert(ind[m_SlicingAxis] - bbox.GetIndex()[m_SlicingAxis]);
                }
                ++it;
            }

            std::copy(unique_slices.begin(), unique_slices.end(), inserter(m_SegmentationIndices, m_SegmentationIndices.begin()));
            for (std::vector<int>::const_iterator s = m_SegmentationIndices.begin(); s != m_SegmentationIndices.end(); ++s)
                std::cout << *s << ' ';

            this->InterpolateLabel(label, m_SegmentationIndices, bbox);
        }
    }
    else {

        //Determine the bounding box for the label of interest
        for ( typename LabeledSlicesType::iterator itS = m_LabeledSlices[m_SlicingAxis].begin();itS != m_LabeledSlices[m_SlicingAxis].end(); ++itS )
        {
            if ( m_Label == itS->first ) { // label needs to be interpolated
                for ( typename BoundingBoxesType::iterator iBB = m_BoundingBoxes.begin();iBB != m_BoundingBoxes.end(); ++iBB )
                {
                    if (m_Label == iBB->first){
                        bbox = iBB->second;
                    }
                }
            }
        }

        //Iterate in the detected slicing direction and include any slice containing the label of interest
        // Added this since DetermineSlicingDirection doesn't always detect the correct slices
        std::set<int> unique_slices;

        ImageRegionConstIteratorWithIndex< TLabelImage > it( SegmentationImage, SegmentationImage->GetRequestedRegion() );
        while ( !it.IsAtEnd() )
        {
            const typename TLabelImage::IndexType ind = it.GetIndex();
            const typename TLabelImage::PixelType val = SegmentationImage->GetPixel( ind );
            if (val == m_Label)
            {
                unique_slices.insert(ind[m_SlicingAxis] - bbox.GetIndex()[m_SlicingAxis]);
            }
            ++it;
        }

        std::copy(unique_slices.begin(), unique_slices.end(), inserter(m_SegmentationIndices, m_SegmentationIndices.begin()));

        this->InterpolateLabel(m_Label, m_SegmentationIndices, bbox);
    }

    // Overwrites m_Output with non non-zeroes from m_Input
    if (m_OverwriteSegmentation){
        ImageRegionIterator< TLabelImage > itO( this->GetInterpolation(), this->GetInterpolation()->GetBufferedRegion() );
        ImageRegionConstIterator< TLabelImage > itI( this->GetSegmentationImage(), this->GetInterpolation()->GetBufferedRegion() );
        while ( !itI.IsAtEnd() )
        {
            typename TLabelImage::PixelType val = itI.Get(); // input segmentation value
            if (val != 0 && itO.Get() == 0) { // Overwrite m_Output with non-zeroes from m_Input which aren't included in the interpolation
                itO.Set( val );
            }

            ++itI;
            ++itO;
        }
    }
    else {
        ImageRegionIterator< TLabelImage > itO( this->GetInterpolation(), this->GetInterpolation()->GetBufferedRegion() );
        ImageRegionConstIterator< TLabelImage > itI( this->GetSegmentationImage(), this->GetInterpolation()->GetBufferedRegion() );
        while ( !itI.IsAtEnd() )
        {
            typename TLabelImage::PixelType val = itI.Get(); // input segmentation value
            if ( val != 0 )
            {
                itO.Set( val );
            }

            ++itI;
            ++itO;
        }

    }

}

} // end namespace

//#endif // ITKBWAANDRFINTERPOLATION_HXX
