#ifndef itkBWAfilter_txx
#define itkBWAfilter_txx

#include <iterator>
#include <vector>
#include "itkObjectFactory.h"
#include "IRISSlicer.h"
#include "itkImage.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "itkRegionOfInterestImageFilter.h"
#include <iostream>

namespace itk
{

template< class TLabelImage, class TMainImage>
DataObject::Pointer BinaryWeightedAveragingFilter <TLabelImage, TMainImage >::MakeOutput(unsigned int idx)
{
    DataObject::Pointer output;

    switch ( idx )
    {
    case 0:
        output = ( TLabelImage::New() ).GetPointer();
        break;
    case 1:
        output = ( TMainImage::New() ).GetPointer();
        break;
    default:
        std::cerr << "No output " << idx << std::endl;
        output = NULL;
        break;
    }
    return output.GetPointer();
}

template< class TLabelImage, class TMainImage >
BinaryWeightedAveragingFilter< TLabelImage, TMainImage >::BinaryWeightedAveragingFilter()
{
    this->SetNumberOfRequiredOutputs(2);
    this->SetNumberOfRequiredInputs(1);
    this->SetNthOutput( 0, this->MakeOutput(0) );
    this->SetNthOutput( 1, this->MakeOutput(1) );
}

template< class TLabelImage, class TMainImage>
TLabelImage* BinaryWeightedAveragingFilter<TLabelImage, TMainImage >::GetInterpolation()
{
    return dynamic_cast< TLabelImage * >(this->ProcessObject::GetOutput(0) );
}

template< class TLabelImage, class TMainImage>
TMainImage* BinaryWeightedAveragingFilter<TLabelImage, TMainImage >::GetProbabilityMap()
{
    return dynamic_cast< TMainImage * >(this->ProcessObject::GetOutput(1) );
}


template< class TLabelImage, class TMainImage >
void BinaryWeightedAveragingFilter< TLabelImage, TMainImage >
::GenerateData()
{
    typename TLabelImage::ConstPointer input = this->GetInput();

    // Setup output 1
    typename TLabelImage::Pointer interpolation = this->GetInterpolation();
    interpolation->SetBufferedRegion( input->GetLargestPossibleRegion() );
    interpolation->Allocate();


    ImageAlgorithm::Copy< TLabelImage, TLabelImage >( input.GetPointer(), interpolation.GetPointer(),
                                                      interpolation->GetRequestedRegion(), interpolation->GetRequestedRegion() );

    // Setup output 2
    typename TMainImage::Pointer probabilitymap = this->GetProbabilityMap();
    probabilitymap->SetBufferedRegion( input->GetLargestPossibleRegion() );
    probabilitymap->Allocate();

    ImageAlgorithm::Copy< TLabelImage, TMainImage >( input.GetPointer(), probabilitymap.GetPointer(),
                                                     probabilitymap->GetRequestedRegion(), probabilitymap->GetRequestedRegion() );

    // Trim image to just the region including the segmentation
    typedef itk::RegionOfInterestImageFilter<TLabelImage, TLabelImage> TrimFilter;
    typename TrimFilter::Pointer TrimImageFilter = TrimFilter::New();
    TrimImageFilter->SetInput(input);
    TrimImageFilter->SetRegionOfInterest(m_boundingbox);
    TrimImageFilter->Update();

    std::vector<int> dimensions;
    dimensions.push_back(0);
    dimensions.push_back(1);
    dimensions.push_back(2);
    dimensions.erase(dimensions.begin() + m_slicingaxis);
    m_firstdirection = dimensions[0];
    m_seconddirection = dimensions[1];

    //Initialize filters to extract slices amd restack to form a 3D volume
    typename TLabelImage::IndexType bbox_index = m_boundingbox.GetIndex();

    typedef IRISSlicer< TLabelImage, TLabelImageSliceType, TLabelImage> IrisSlicerFilterType;
    typename IrisSlicerFilterType::Pointer Slicer[2];

    itk::FixedArray< unsigned int, 3 > layout;
    layout[0] = 1;
    layout[1] = 1;
    layout[2] = 0; // Set third element to zero to get a 3D volume [1,1,0]

    typename TLabelImage::PixelType filler = 0;
    typename TMainImage::PixelType filler_double = 0;

    //Get total number of slices to iterate through
    int totalSlices = 0;
    for (auto it = m_SegmentationIndices.begin(); it != m_SegmentationIndices.end(); ++it) {
        totalSlices += 1;
    }

    std::vector<typename AndImageFilterType::Pointer> Intersect(totalSlices-1);
    std::vector<typename SignedDistanceMapFilterType::Pointer> SignedDistanceMapIntersect(totalSlices - 1);

    //Loop through set
    unsigned int i = 0; // to keep track of slice index?
    for (auto it_first = m_SegmentationIndices.begin(); it_first != std::prev(m_SegmentationIndices.end()); it_first++) {

        auto first_sliceindex = *it_first - bbox_index[m_slicingaxis]; // Indices relative to bounding box, not whole image

        auto it_second = std::next(it_first);
        auto second_sliceindex = *it_second - bbox_index[m_slicingaxis];

        const int numSlices = second_sliceindex - first_sliceindex;

        if(numSlices > 1){ // Only if the slices are not adjacent - DetermineSliceOrientation can detect adjacent slices sometimes

            int intermediate_slice = numSlices/2;

            // Extract the first slice
            Slicer[0] = IrisSlicerFilterType::New();
            Slicer[0]->SetInput(TrimImageFilter->GetOutput());
            Slicer[0]->SetSliceIndex(first_sliceindex); // -1 in old version
            Slicer[0]->SetSliceDirectionImageAxis(m_slicingaxis);
            Slicer[0]->SetLineDirectionImageAxis(m_seconddirection);
            Slicer[0]->SetPixelDirectionImageAxis(m_firstdirection);
            Slicer[0]->SetLineTraverseForward(0);
            Slicer[0]->SetPixelTraverseForward(0);
            Slicer[0]->Update();

            // Extract the second slice
            Slicer[1] = IrisSlicerFilterType::New();
            Slicer[1]->SetInput(TrimImageFilter->GetOutput());
            Slicer[1]->SetSliceIndex(second_sliceindex); //-1 in old version
            Slicer[1]->SetSliceDirectionImageAxis(m_slicingaxis);
            Slicer[1]->SetLineDirectionImageAxis(m_seconddirection);
            Slicer[1]->SetPixelDirectionImageAxis(m_firstdirection);
            Slicer[1]->SetLineTraverseForward(0);
            Slicer[1]->SetPixelTraverseForward(0);
            Slicer[1]->Update();

            // Compute connected components

            //Compute A intersect B
            Intersect[i] = AndImageFilterType::New();
            Intersect[i]->SetInput1(Slicer[0]->GetOutput());
            Intersect[i]->SetInput2(Slicer[1]->GetOutput());
            Intersect[i]->Update();

            // Check whether consecutive segmentations are intersecting
            itk::ImageRegionConstIterator<TLabelImageSliceType> It(Intersect[i]->GetOutput(),Intersect[i]->GetOutput()->GetLargestPossibleRegion());
            int sum = 0;
            It.GoToBegin();
            while (!It.IsAtEnd())
            {

                sum = sum + It.Get();
                ++It;
            }
            if(sum ==0){
                throw itk::ExceptionObject(
                    "Error: consecutive segmentations must be intersecting for "
                    "Binary Weighted Averaging. Manually specifying the axis "
                    "of interpolation may resolve this error.");
            }

            //Compute the Signed Distance Map of the intersecting portion
            SignedDistanceMapIntersect[i] = SignedDistanceMapFilterType::New();
            SignedDistanceMapIntersect[i]->SetInput(Intersect[i]->GetOutput());
            SignedDistanceMapIntersect[i]->UseImageSpacingOff();
            SignedDistanceMapIntersect[i]->SquaredDistanceOff();
            SignedDistanceMapIntersect[i]->InsideIsPositiveOn();
            SignedDistanceMapIntersect[i]->Update();

            typename SubtractImageFilterType::Pointer DifferenceImage[2];
            // Compute A\B
            DifferenceImage[0] = SubtractImageFilterType::New();
            DifferenceImage[0]->SetInput1(Slicer[0]->GetOutput());
            DifferenceImage[0]->SetInput2(Intersect[i]->GetOutput());
            DifferenceImage[0]->Update();
            typename TLabelImageSliceType::Pointer AdiffB = DifferenceImage[0]->GetOutput();
            AdiffB->DisconnectPipeline();

            // Compute B\A
            DifferenceImage[1] = SubtractImageFilterType::New();
            DifferenceImage[1]->SetInput1(Slicer[1]->GetOutput());
            DifferenceImage[1]->SetInput2(Intersect[i]->GetOutput());
            DifferenceImage[1]->Update();
            typename TLabelImageSliceType::Pointer BdiffA = DifferenceImage[1]->GetOutput();
            BdiffA->DisconnectPipeline();

            //Calculate the connected components  in A \ B
            typename ConnectedComponentImageFilterType::Pointer ConnectedAdifB = ConnectedComponentImageFilterType::New ();
            ConnectedAdifB->SetInput(AdiffB); // A\B
            ConnectedAdifB->Update();

            int numCCAdB = ConnectedAdifB->GetObjectCount();
            std::vector<typename BinaryThresholdImageFilterType::Pointer> ConnectedComponent_AdB(numCCAdB);
            std::vector<typename OrImageFilterType::Pointer> A_dash(numCCAdB);

            //Initialize filters and varaibles to combine results across connected components

            typename TLabelImage::Pointer InterpolatedVolume[2];
            typename TMainImage::Pointer ProbabilityMap[2];
            typename TLabelImageSliceType::Pointer InterpolatedVolume_intermediate[2];
            typename TMainImageSliceType::Pointer ProbabilityMap_intermediate[2];
            std::vector<typename TilerType::Pointer> Tiler_Interp(numCCAdB);
            std::vector<typename DoubleTilerType::Pointer> Tiler_ERF(numCCAdB);
            std::vector<typename SignedDistanceMapFilterType::Pointer> SignedDistanceMapA_dash(numCCAdB);

            itk::FixedArray<bool, TMainImage::ImageDimension > flipAxes;
            flipAxes[m_firstdirection] = true;
            flipAxes[m_seconddirection] = true;
            flipAxes[m_slicingaxis] = false;

            itk::FixedArray<bool, TMainImageSliceType::ImageDimension > flipAxesSliceType;
            flipAxesSliceType[0] = true;
            flipAxesSliceType[1] = true;

            //Loop through connected components in A\B

            for( int j = 0; j < numCCAdB; j++ )
            {

                //Isolate single connected component
                ConnectedComponent_AdB[j] = BinaryThresholdImageFilterType::New();
                ConnectedComponent_AdB[j]->SetInput(ConnectedAdifB->GetOutput());
                ConnectedComponent_AdB[j]->SetLowerThreshold(j+1);
                ConnectedComponent_AdB[j]->SetUpperThreshold(j+1);
                ConnectedComponent_AdB[j]->SetInsideValue(1);
                ConnectedComponent_AdB[j]->SetOutsideValue(0);
                ConnectedComponent_AdB[j]->Update();

                // A' = (A \intersect B) \u C
                A_dash[j] = OrImageFilterType::New();
                A_dash[j]->SetInput1(ConnectedComponent_AdB[j]->GetOutput());
                A_dash[j]->SetInput2(Intersect[i]->GetOutput());
                A_dash[j]->Update();

                SignedDistanceMapA_dash[j] = SignedDistanceMapFilterType::New();
                SignedDistanceMapA_dash[j]->SetInput(A_dash[j]->GetOutput());
                SignedDistanceMapA_dash[j]->UseImageSpacingOff();
                SignedDistanceMapA_dash[j]->SquaredDistanceOff();
                SignedDistanceMapA_dash[j]->InsideIsPositiveOn();
                SignedDistanceMapA_dash[j]->Update();

                // Calculate the reparameterization function g

                // First calculate the area of difference set A\B
                double AreaAB = 0;
                itk::ImageRegionConstIterator<TLabelImageSliceType> It(ConnectedComponent_AdB[j]->GetOutput(),ConnectedComponent_AdB[j]->GetOutput()->GetLargestPossibleRegion());
                It.GoToBegin();
                while (!It.IsAtEnd())
                {
                    if (It.Get()!=0) //Or whatever value is your white pixel
                    {
                        AreaAB+=1;
                    }
                    ++It;
                }

                // Compute the reparameterization using an iterator

                // Loop through different values of the parameter t and compute the vaule of g
                const int numElements = 60;
                double gAB [numElements];

                for (int t = 0;t < numElements; t++){

                    double I_diffB = 0;
                    double B_diffI = 0;
                    double AreaIB = 0;
                    double t1 = t*(double(1)/(numElements-1));

                    itk::ImageRegionConstIterator<TMainImageSliceType> It_sdtA(SignedDistanceMapA_dash[j]->GetOutput(),SignedDistanceMapA_dash[j]->GetOutput()->GetLargestPossibleRegion());
                    itk::ImageRegionConstIterator<TMainImageSliceType> It_sdtIntersect(SignedDistanceMapIntersect[i]->GetOutput(),SignedDistanceMapIntersect[i]->GetOutput()->GetLargestPossibleRegion());
                    itk::ImageRegionConstIterator<TLabelImageSliceType> ItB(Intersect[i]->GetOutput(),Intersect[i]->GetOutput()->GetLargestPossibleRegion());

                    It_sdtA.GoToBegin();
                    It_sdtIntersect.GoToBegin();
                    ItB.GoToBegin();

                    while (!It_sdtA.IsAtEnd())
                    {
                        double x1 = It_sdtA.Get();
                        double x2 = It_sdtIntersect.Get();
                        double interp = (1-t1)*x1 + t1*x2; // interpolate between the two pixels
                        int B = ItB.Get();

                        if (interp >= 0 && B == 0)
                        {
                            I_diffB+=1;
                        }
                        if (interp < 0 && B == 1)
                        {
                            B_diffI+=1;
                        }

                        ++It_sdtA;\
                        ++It_sdtIntersect;
                        ++ItB;
                    }

                    AreaIB = I_diffB + B_diffI;
                    double g = AreaIB/AreaAB;
                    gAB[t] = 1 - g;

                } // End of loop through different paramter values - finished computing gAB

                //Given the reparametrization function, can now interpolate between the different t-values

                // Loop through averaging paramter for interpolation

                if (m_intermediateslices == false){

                    Tiler_Interp[j] = TilerType::New();
                    Tiler_Interp[j]->SetLayout(layout);
                    Tiler_ERF[j] = DoubleTilerType::New();
                    Tiler_ERF[j]->SetLayout(layout);
                    Tiler_Interp[j]->SetDefaultPixelValue( filler );
                    Tiler_ERF[j]->SetDefaultPixelValue( filler_double );

                    typename InterpolationFilterType:: Pointer ComputeInterpolation;

                    for (int t = 1;t < numSlices; t++){

                        double t1 =  t*(double(1)/numSlices);

                        // Find the reparametrization using inverse of gAB
                        double tdash_max, tdash_min, tdash;
                        for (int n = 0; n < numElements-1; n++){
                            double  gmin_low = gAB[n] - t1;
                            double gmin_high = gAB[n+1] - t1;

                            if ( (gmin_low <= 0) && (gmin_high >= 0)){
                                tdash_min =  n*(double(1)/(numElements-1));
                                tdash_max = (n+1)*(double(1)/(numElements-1));
                                double dist_low = 0 - gmin_low;
                                double dist_high = gmin_high - 0;
                                tdash = dist_high/(dist_low + dist_high)*tdash_min +  dist_low/(dist_low + dist_high)*tdash_max;
                            }
                        }
                        ComputeInterpolation = InterpolationFilterType::New();
                        ComputeInterpolation->SetInputImage1(SignedDistanceMapA_dash[j]->GetOutput());
                        ComputeInterpolation->SetInputImage2(SignedDistanceMapIntersect[i]->GetOutput());
                        ComputeInterpolation->SetConstant(tdash);
                        ComputeInterpolation->Update();

                        typename  TLabelImageSliceType::Pointer interp = ComputeInterpolation->GetInterpolation();
                        interp->DisconnectPipeline();
                        typename TMainImageSliceType::Pointer pmap = ComputeInterpolation->GetProbabilityMap();
                        pmap->DisconnectPipeline();

                        Tiler_Interp[j]->SetInput(t-1,interp);
                        Tiler_ERF[j]->SetInput(t-1,pmap);

                    } // End of loop through interpolation using different t values

                    Tiler_Interp[j]->Update();
                    Tiler_ERF[j]->Update();

                    if(j == 0){ //for the first connected component

                        InterpolatedVolume[0] = Tiler_Interp[j]->GetOutput();
                        ProbabilityMap[0] = Tiler_ERF[j]->GetOutput();
                        InterpolatedVolume[0]->DisconnectPipeline();
                        ProbabilityMap[0]->DisconnectPipeline();
                    }
                    else{

                        typename ThreeDOrImageFilterType::Pointer CombineComponentsFilterAdB = ThreeDOrImageFilterType::New();
                        CombineComponentsFilterAdB->SetInput1(InterpolatedVolume[0]);
                        CombineComponentsFilterAdB->SetInput2(Tiler_Interp[j]->GetOutput());
                        CombineComponentsFilterAdB->Update();
                        InterpolatedVolume[0] = CombineComponentsFilterAdB->GetOutput();
                        InterpolatedVolume[0]->DisconnectPipeline();

                        typename MaximumImageFilterType::Pointer MaximumImageFilterAdB = MaximumImageFilterType::New ();
                        MaximumImageFilterAdB->SetInput(0, ProbabilityMap[0]);
                        MaximumImageFilterAdB->SetInput(1, Tiler_ERF[j]->GetOutput());
                        MaximumImageFilterAdB ->Update();
                        ProbabilityMap[0] = MaximumImageFilterAdB->GetOutput();
                        ProbabilityMap[0]->DisconnectPipeline();
                    }
                } // m_intermediateslices == false

                if(m_intermediateslices == true){

                    double t1 = double(intermediate_slice)/numSlices;
                    typename InterpolationFilterType:: Pointer ComputeInterpolation;

                    // Find the reparametrization using inverse of gAB
                    double tdash_max, tdash_min, tdash;
                    for (int n = 0; n < numElements-1; n++){
                        double  gmin_low = gAB[n] - t1;
                        double gmin_high = gAB[n+1] - t1;

                        if ( (gmin_low <= 0) && (gmin_high >= 0)){
                            tdash_min =  n*(double(1)/(numElements-1));
                            tdash_max = (n+1)*(double(1)/(numElements-1));
                            double dist_low = 0 - gmin_low;
                            double dist_high = gmin_high - 0;
                            tdash = dist_high/(dist_low + dist_high)*tdash_min +  dist_low/(dist_low + dist_high)*tdash_max;
                        }
                        ComputeInterpolation = InterpolationFilterType::New();
                        ComputeInterpolation->SetInputImage1(SignedDistanceMapA_dash[j]->GetOutput());
                        ComputeInterpolation->SetInputImage2(SignedDistanceMapIntersect[i]->GetOutput());
                        ComputeInterpolation->SetConstant(tdash);
                        ComputeInterpolation->Update();
                    }

                    if(j == 0){
                        InterpolatedVolume_intermediate[0] = ComputeInterpolation->GetInterpolation();
                        ProbabilityMap_intermediate[0] = ComputeInterpolation->GetProbabilityMap();
                        InterpolatedVolume_intermediate[0]->DisconnectPipeline();
                        ProbabilityMap_intermediate[0]->DisconnectPipeline();
                    }
                    else{
                        typename OrImageFilterType::Pointer CombineComponentsFilterAdB = OrImageFilterType::New();
                        CombineComponentsFilterAdB->SetInput1(InterpolatedVolume_intermediate[0]);
                        CombineComponentsFilterAdB->SetInput2(ComputeInterpolation->GetInterpolation());
                        CombineComponentsFilterAdB->Update();
                        InterpolatedVolume_intermediate[0] = CombineComponentsFilterAdB->GetOutput();

                        typename T2DMaximumImageFilterType::Pointer MaximumImageFilterAdB = T2DMaximumImageFilterType::New ();
                        MaximumImageFilterAdB->SetInput(0, ProbabilityMap_intermediate[0]);
                        MaximumImageFilterAdB->SetInput(1, ComputeInterpolation->GetProbabilityMap());
                        MaximumImageFilterAdB->Update();
                        ProbabilityMap_intermediate[0] = MaximumImageFilterAdB->GetOutput();

                        InterpolatedVolume_intermediate[0]->DisconnectPipeline();
                        ProbabilityMap_intermediate[0]->DisconnectPipeline();
                    }
                }

            } // End of loop through connected components in A\B

            //  **********************************************

            // Initialize filters and varaibled need to combine results across connected components
            typename ConnectedComponentImageFilterType::Pointer ConnectedBdifA = ConnectedComponentImageFilterType::New ();
            ConnectedBdifA->SetInput(BdiffA); // B\A
            ConnectedBdifA->Update();

            int numCCBdA = ConnectedBdifA->GetObjectCount();
            std::vector<typename BinaryThresholdImageFilterType::Pointer> ConnectedComponent_BdA(numCCBdA);
            std::vector<typename OrImageFilterType::Pointer> B_dash(numCCBdA);
            std::vector<typename SignedDistanceMapFilterType::Pointer> SignedDistanceMapB_dash(numCCBdA);

            std::vector<typename TilerType::Pointer> TilerBdA_Interp(numCCBdA);
            std::vector<typename DoubleTilerType::Pointer> TilerBdA_ERF(numCCBdA);

            //  Calculate the connected components  in B\A
            for( int j = 0; j < numCCBdA; j++ )
            {
                // Isolate single connected component
                ConnectedComponent_BdA[j] = BinaryThresholdImageFilterType::New();
                ConnectedComponent_BdA[j]->SetInput(ConnectedBdifA->GetOutput());
                ConnectedComponent_BdA[j]->SetLowerThreshold(j+1);
                ConnectedComponent_BdA[j]->SetUpperThreshold(j+1);
                ConnectedComponent_BdA[j]->SetInsideValue(1);
                ConnectedComponent_BdA[j]->SetOutsideValue(0);
                ConnectedComponent_BdA[j]->Update();

                //B' = (A \intersect B) \u C
                B_dash[j] = OrImageFilterType::New();
                B_dash[j]->SetInput1(ConnectedComponent_BdA[j]->GetOutput());
                B_dash[j]->SetInput2(Intersect[i]->GetOutput());
                B_dash[j]->Update();

                SignedDistanceMapB_dash[j] = SignedDistanceMapFilterType::New();
                SignedDistanceMapB_dash[j]->SetInput(B_dash[j]->GetOutput());
                SignedDistanceMapB_dash[j]->UseImageSpacingOff();
                SignedDistanceMapB_dash[j]->SquaredDistanceOff();
                SignedDistanceMapB_dash[j]->InsideIsPositiveOn();
                SignedDistanceMapB_dash[j]->Update();

                //  Calculate the reparameterization function g

                // Calculate the area of the sym diff between A' and B' - which is the area of C

                // Calcualte the area of the difference set
                double AreaAB = 0;
                itk::ImageRegionConstIterator< TLabelImageSliceType > It(ConnectedComponent_BdA[j]->GetOutput(),ConnectedComponent_BdA[j]->GetOutput()->GetLargestPossibleRegion());
                It.GoToBegin();
                while (!It.IsAtEnd())
                {
                    if (It.Get()!=0) //Or whatever value is your white pixel
                    {
                        AreaAB+=1;
                    }
                    ++It;
                }

                // Loop through different values of the parameter t and compute the vaule of g
                const int numElements = 60;
                double gAB [numElements];

                for (int t = 0;t < numElements; t++){

                    double t1 =  t*(double(1)/(numElements-1));
                    double I_diffB = 0;
                    double B_diffI = 0;
                    double AreaIB = 0;

                    itk::ImageRegionConstIterator<TMainImageSliceType> It_sdtIntersect(SignedDistanceMapIntersect[i]->GetOutput(),SignedDistanceMapIntersect[i]->GetOutput()->GetLargestPossibleRegion());
                    itk::ImageRegionConstIterator<TMainImageSliceType> It_sdtB(SignedDistanceMapB_dash[j]->GetOutput(),SignedDistanceMapB_dash[j]->GetOutput()->GetLargestPossibleRegion());
                    itk::ImageRegionConstIterator<TLabelImageSliceType> ItB(B_dash[j]->GetOutput(),B_dash[j]->GetOutput()->GetLargestPossibleRegion());

                    It_sdtIntersect.GoToBegin();
                    It_sdtB.GoToBegin();
                    ItB.GoToBegin();

                    while (!It_sdtIntersect.IsAtEnd())
                    {
                        double x1 = It_sdtIntersect.Get();
                        double x2 = It_sdtB.Get();

                        double interp = (1-t1)*x1 + t1*x2; // interpolate between the two pixels
                        int B = ItB.Get();

                        if (interp >= 0 && B == 0)
                        {
                            I_diffB+=1;
                        }
                        if (interp < 0 && B == 1)
                        {
                            B_diffI+=1;
                        }

                        ++It_sdtB;\
                        ++It_sdtIntersect;
                        ++ItB;
                    }
                    AreaIB = I_diffB + B_diffI;
                    double g = AreaIB/AreaAB;
                    gAB[t] = 1 - g;

                } // End of loop through different paramter values - finished computing gAB

                // Given the reparametrization function, can now interpolate between the different t-values

                // Loop through averaging paramter for interpolation

                if ( m_intermediateslices == false){

                    TilerBdA_Interp[j] = TilerType::New();
                    TilerBdA_Interp[j]->SetLayout(layout);
                    TilerBdA_ERF[j] = DoubleTilerType::New();
                    TilerBdA_ERF[j]->SetLayout(layout);
                    TilerBdA_ERF[j]->SetDefaultPixelValue( filler_double );
                    TilerBdA_Interp[j]->SetDefaultPixelValue( filler );

                    typename InterpolationFilterType:: Pointer ComputeInterpolation2;
                    typename TLabelImageSliceType::Pointer interp2;
                    typename TMainImageSliceType::Pointer pmap2;

                    for (int t = 1;t < numSlices; t++){

                        double t1 = t*(double(1)/numSlices);

                        // Find the reparametrization using inverse of gAB

                        double tdash_max, tdash_min, tdash;
                        for (int n = 0; n < numElements-1; n++){
                            double  gmin_low = gAB[n] - t1;
                            double gmin_high = gAB[n+1] - t1;

                            if ( (gmin_low <= 0) && (gmin_high >= 0)){

                                tdash_min = n*(double(1)/numElements);
                                tdash_max = (n+1)*(double(1)/numElements);
                                double dist_low = 0 - gmin_low;
                                double dist_high = gmin_high - 0;
                                tdash = dist_high/(dist_low + dist_high)*tdash_min +  dist_low/(dist_low + dist_high)*tdash_max;
                            }
                        }

                        ComputeInterpolation2 = InterpolationFilterType::New();
                        ComputeInterpolation2->SetInputImage1(SignedDistanceMapIntersect[i]->GetOutput());
                        ComputeInterpolation2->SetInputImage2(SignedDistanceMapB_dash[j]->GetOutput());
                        ComputeInterpolation2->SetConstant(tdash);
                        ComputeInterpolation2->Update();
                        interp2 = ComputeInterpolation2->GetInterpolation();
                        interp2->DisconnectPipeline();
                        pmap2 = ComputeInterpolation2->GetProbabilityMap();
                        pmap2->DisconnectPipeline();

                        TilerBdA_Interp[j]->SetInput(t-1,interp2);

                        TilerBdA_ERF[j]->SetInput(t-1,pmap2);

                    } // Loop through interpolation using different t values

                    TilerBdA_Interp[j]->Update();
                    TilerBdA_ERF[j]->Update();

                    if(j == 0){
                        InterpolatedVolume[1] = TilerBdA_Interp[j]->GetOutput();
                        ProbabilityMap[1] = TilerBdA_ERF[j]->GetOutput();
                        InterpolatedVolume[1]->DisconnectPipeline();
                        ProbabilityMap[1]->DisconnectPipeline();
                    }
                    else{
                        typename ThreeDOrImageFilterType::Pointer CombineComponentsFilterBdA = ThreeDOrImageFilterType::New();
                        CombineComponentsFilterBdA->SetInput1(InterpolatedVolume[1]);
                        CombineComponentsFilterBdA->SetInput2(TilerBdA_Interp[j]->GetOutput());
                        CombineComponentsFilterBdA->Update();
                        InterpolatedVolume[1] = CombineComponentsFilterBdA->GetOutput();

                        typename MaximumImageFilterType::Pointer MaximumImageFilterBdA = MaximumImageFilterType::New();
                        MaximumImageFilterBdA->SetInput(0, ProbabilityMap[1]);
                        MaximumImageFilterBdA->SetInput(1, TilerBdA_ERF[j]->GetOutput());
                        MaximumImageFilterBdA->Update();
                        ProbabilityMap[1] = MaximumImageFilterBdA->GetOutput();

                        InterpolatedVolume[1]->DisconnectPipeline();
                        ProbabilityMap[1]->DisconnectPipeline();
                    }
                }

                if ( m_intermediateslices == true){ // Interpolate only middle slices

                    typename InterpolationFilterType:: Pointer ComputeInterpolation2;
                    double t1 = double(intermediate_slice)/numSlices;

                    // Find the reparametrization using inverse of gAB
                    double tdash_max, tdash_min, tdash;
                    for (int n = 0; n < numElements-1; n++){
                        double  gmin_low = gAB[n] - t1;
                        double gmin_high = gAB[n+1] - t1;

                        if ( (gmin_low <= 0) && (gmin_high >= 0)){
                            tdash_min =  n*(double(1)/numElements);
                            tdash_max = (n+1)*(double(1)/numElements);
                            double dist_low = 0 - gmin_low;
                            double dist_high = gmin_high - 0;
                            tdash = dist_high/(dist_low + dist_high)*tdash_min +  dist_low/(dist_low + dist_high)*tdash_max;
                        }
                    }
                    ComputeInterpolation2 = InterpolationFilterType::New();
                    ComputeInterpolation2->SetInputImage1(SignedDistanceMapIntersect[i]->GetOutput());
                    ComputeInterpolation2->SetInputImage2(SignedDistanceMapB_dash[j]->GetOutput());
                    ComputeInterpolation2->SetConstant(tdash);
                    ComputeInterpolation2->Update();

                    if(j == 0){

                        InterpolatedVolume_intermediate[1] = ComputeInterpolation2->GetInterpolation();
                        InterpolatedVolume_intermediate[1]->DisconnectPipeline();
                        ProbabilityMap_intermediate[1] = ComputeInterpolation2->GetProbabilityMap();
                        ProbabilityMap_intermediate[1]->DisconnectPipeline();
                    }
                    else{

                        typename OrImageFilterType::Pointer CombineComponentsFilterBdA = OrImageFilterType::New();
                        CombineComponentsFilterBdA->SetInput1(InterpolatedVolume_intermediate[1]);
                        CombineComponentsFilterBdA->SetInput2(ComputeInterpolation2->GetInterpolation());
                        CombineComponentsFilterBdA->Update();
                        InterpolatedVolume_intermediate[1] = CombineComponentsFilterBdA->GetOutput();
                        InterpolatedVolume_intermediate[1]->DisconnectPipeline();

                        typename T2DMaximumImageFilterType::Pointer MaximumImageFilterBdA = T2DMaximumImageFilterType::New ();
                        MaximumImageFilterBdA->SetInput(0, ProbabilityMap_intermediate[1]);
                        MaximumImageFilterBdA->SetInput(1, ComputeInterpolation2->GetProbabilityMap());
                        MaximumImageFilterBdA->Update();
                        ProbabilityMap_intermediate[1] = MaximumImageFilterBdA->GetOutput();
                        ProbabilityMap_intermediate[1]->DisconnectPipeline();
                    }
                }

            } // End of loop through connected components in B\A

            if ( m_intermediateslices == false){


                typename ThreeDOrImageFilterType::Pointer CombineInterpolations = ThreeDOrImageFilterType::New();
                CombineInterpolations->SetInput1(InterpolatedVolume[0]);
                CombineInterpolations->SetInput2(InterpolatedVolume[1]);
                CombineInterpolations->Update();
                typename TLabelImage::Pointer interpolated_segment = CombineInterpolations->GetOutput();
                interpolated_segment->DisconnectPipeline();

                typename MaximumImageFilterType::Pointer CombineProbabilityMaps = MaximumImageFilterType::New ();
                CombineProbabilityMaps->SetInput(0, ProbabilityMap[0]);
                CombineProbabilityMaps->SetInput(1, ProbabilityMap[1]);
                CombineProbabilityMaps->Update();
                typename TMainImage::Pointer pmap_segment = CombineProbabilityMaps->GetOutput();
                pmap_segment->DisconnectPipeline();

                typename PermuteAxesImageFilterType::Pointer permute_axes = PermuteAxesImageFilterType::New();
                permute_axes->SetInput(interpolated_segment);

                typename PermuteAxesImageFilterType::PermuteOrderArrayType order;
                order[m_firstdirection] = 0;
                order[m_seconddirection] = 1;
                order[m_slicingaxis] = 2;
                permute_axes->SetOrder(order);

                typename FlipImageFilterType::Pointer flip_image = FlipImageFilterType::New();
                flip_image->SetInput(permute_axes->GetOutput());
                flip_image->SetFlipAxes(flipAxes);
                flip_image->Update();
                typename TLabelImage::Pointer permuted_region = flip_image->GetOutput();

                //Fixed this code
                typename TLabelImage::IndexType newRegionIndex = bbox_index;
                newRegionIndex[m_slicingaxis] = bbox_index[m_slicingaxis] + first_sliceindex +1;

                typename TLabelImage::RegionType newRegion = permuted_region->GetLargestPossibleRegion();
                newRegion.SetIndex(newRegionIndex);

                ImageAlgorithm::Copy< TLabelImage, TLabelImage >( permuted_region.GetPointer(), interpolation.GetPointer(),
                                                                  permuted_region->GetLargestPossibleRegion(), newRegion);

                // Copy probability map
                typename PermuteAxesDoubleImageFilterType::Pointer permute_axes_double = PermuteAxesDoubleImageFilterType::New();
                permute_axes_double->SetInput(pmap_segment);
                typename PermuteAxesDoubleImageFilterType::PermuteOrderArrayType pmap_order;
                order[m_firstdirection] = 0;
                order[m_seconddirection] = 1;
                order[m_slicingaxis] = 2;
                permute_axes_double->SetOrder( order );

                typename FlipDoubleImageFilterType::Pointer flip_double_image = FlipDoubleImageFilterType::New();
                flip_double_image->SetInput(permute_axes_double->GetOutput());
                flip_double_image->SetFlipAxes(flipAxes);
                flip_double_image->Update();

                typename TMainImage::Pointer permuted_double_region = flip_double_image->GetOutput();

                 //Define new region to copy interpolation into
                typename TMainImage::IndexType newRegionIndex_double = bbox_index;
                newRegionIndex_double[m_slicingaxis] = bbox_index[m_slicingaxis] + first_sliceindex +1;
                typename TMainImage::RegionType newRegion_double = permuted_double_region->GetLargestPossibleRegion();
                newRegion_double.SetIndex(newRegionIndex_double) ;

                ImageAlgorithm::Copy< TMainImage, TMainImage >( permuted_double_region.GetPointer(), probabilitymap.GetPointer(),
                                                                permuted_double_region->GetLargestPossibleRegion(), newRegion_double);
            }

            if ( m_intermediateslices == true){

                typename OrImageFilterType::Pointer CombineInterpolations = OrImageFilterType::New();
                CombineInterpolations->SetInput1(InterpolatedVolume_intermediate[0]);
                CombineInterpolations->SetInput2(InterpolatedVolume_intermediate[1]);
                CombineInterpolations->Update();
                typename TLabelImageSliceType::Pointer interpolated_segment = CombineInterpolations->GetOutput();
                interpolated_segment->DisconnectPipeline();

                typename T2DMaximumImageFilterType::Pointer CombineProbabilityMaps = T2DMaximumImageFilterType::New ();
                CombineProbabilityMaps->SetInput(0, ProbabilityMap_intermediate[0]);
                CombineProbabilityMaps->SetInput(1, ProbabilityMap_intermediate[1]);
                CombineProbabilityMaps->Update();
                typename TMainImageSliceType::Pointer pmap_segment = CombineProbabilityMaps->GetOutput();
                pmap_segment->DisconnectPipeline();

                typename FlipSliceImageFilterType::Pointer flip_image = FlipSliceImageFilterType::New();
                flip_image->SetInput(interpolated_segment);
                flip_image->SetFlipAxes(flipAxesSliceType);
                flip_image->Update();
                typename TLabelImageSliceType::Pointer permuted_region = flip_image->GetOutput();

                //Define new Region to copy interpolated result into
                typename TLabelImage::IndexType newRegionIndex = bbox_index;
                newRegionIndex[m_slicingaxis] = bbox_index[m_slicingaxis] + first_sliceindex + intermediate_slice;

                typename TLabelImage::SizeType newRegionSize = m_boundingbox.GetSize();
                newRegionSize[m_slicingaxis] = 1;

                typename TMainImage::RegionType newRegion;
                newRegion.SetSize(newRegionSize);
                newRegion.SetIndex(newRegionIndex);

                ImageAlgorithm::Copy< TLabelImageSliceType, TLabelImage >( permuted_region.GetPointer(), interpolation.GetPointer(),
                                                                           permuted_region->GetLargestPossibleRegion(), newRegion);

                // Copy probability map
                typename FlipDoubleSliceImageFilterType::Pointer flip_double_image = FlipDoubleSliceImageFilterType::New();
                flip_double_image->SetInput(pmap_segment);
                flip_double_image->SetFlipAxes(flipAxesSliceType);
                flip_double_image->Update();
                typename TMainImageSliceType::Pointer permuted_double_region = flip_double_image->GetOutput();

                typename TMainImage::IndexType newRegionIndex_double = bbox_index;
                newRegionIndex_double[m_slicingaxis] = bbox_index[m_slicingaxis] + first_sliceindex + intermediate_slice;
                typename TMainImage::RegionType newRegion_double;
                typename TMainImage::SizeType newRegionSize_double = m_boundingbox.GetSize();
                newRegionSize_double[m_slicingaxis] = 1;
                newRegion_double.SetSize(newRegionSize_double);
                newRegion_double.SetIndex(newRegionIndex_double) ;

                ImageAlgorithm::Copy< TMainImageSliceType, TMainImage >( permuted_double_region.GetPointer(), probabilitymap.GetPointer(),
                                                                         permuted_double_region->GetLargestPossibleRegion(), newRegion_double);
            }

        } // end of 'if segmented slices are not consecutive'
        else{
            continue;
        } // Slices are consecutive
        i += 1; // Increment slice counter
    } // End of loop through slices

} // End of GenerateData()

} // End of itk namespace

#endif // itkBWAandRFinterpolation_txx

