#include "RLEImageRegionIterator.h"
#include "RLERegionOfInterestImageFilter.h"
#include <iostream>
#include <string>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkTimeProbe.h>
#include "IRISSlicer.h"
#include "itkTestingComparisonImageFilter.h"

//using namespace std;

typedef itk::Image<short, 3> Seg3DImageType;
typedef itk::Image<short, 2> Seg2DImageType;

typedef RLEImage<short> shortRLEImage;
typedef itk::RegionOfInterestImageFilter<shortRLEImage, shortRLEImage> roiType;

Seg3DImageType::Pointer loadImage(const std::string filename)
{
    typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
    SegReaderType::Pointer sr = SegReaderType::New();
    sr->SetFileName(filename);
    sr->Update();
    return sr->GetOutput();
}

void writeImage(Seg2DImageType::Pointer image, const std::string filename, bool compress = true)
{
    typedef itk::ImageFileWriter<Seg2DImageType> SegWriterType;
    SegWriterType::Pointer sw = SegWriterType::New();
    sw->SetInput(image);
    sw->SetFileName(filename);
    sw->SetUseCompression(compress);
    sw->Update();
}

void writeImage(Seg3DImageType::Pointer image, const std::string filename, bool compress = true)
{
    typedef itk::ImageFileWriter<Seg3DImageType> SegWriterType;
    SegWriterType::Pointer sw = SegWriterType::New();
    sw->SetInput(image);
    sw->SetFileName(filename);
    sw->SetUseCompression(compress);
    sw->Update();
}

inline char axisToLetter(unsigned axis)
{
    if (axis == 0)
        return 'X';
    else if (axis == 1)
        return 'Y';
    else if (axis == 2)
        return 'Z';
    else
        return '?';
}

//invokes IRISSlicer<itk> and IRISSlicer<rle> and compares results
void testIRISSlicer(shortRLEImage::Pointer rleImage, Seg3DImageType::Pointer itkImage,
    unsigned sliceIndex, unsigned sliceAxis, unsigned lineAxis, unsigned pixelAxis,
    bool lineForward, bool pixelForward)
{
    itk::TimeProbe tp;
    std::cout << "Axes (slice/line/pixel): " << axisToLetter(sliceAxis) << '/'
        << axisToLetter(lineAxis) << '/' << axisToLetter(pixelAxis)  << std::endl;
    std::cout << "Slice index: " << sliceIndex << ".  Line forward: " << lineForward
        << ".  Pixel forward: " << pixelForward << std::endl;;

    std::cout << "IRISSlicer<rle>: "; tp.Start();
    typedef IRISSlicer<shortRLEImage, Seg2DImageType, shortRLEImage> slicerTypeRLE;
    slicerTypeRLE::Pointer roiRLE = slicerTypeRLE::New();
    roiRLE->SetInput(rleImage);
    roiRLE->SetSliceIndex(sliceIndex);
    roiRLE->SetSliceDirectionImageAxis(sliceAxis);
    roiRLE->SetLineDirectionImageAxis(lineAxis);
    roiRLE->SetPixelDirectionImageAxis(pixelAxis);
    roiRLE->SetLineTraverseForward(lineForward);
    roiRLE->SetPixelTraverseForward(pixelForward);
    roiRLE->Update();
    Seg2DImageType::Pointer sliceRLE = roiRLE->GetOutput();
    tp.Stop(); std::cout << tp.GetMean() * 1000 << " ms " << std::endl; tp.Reset();

    std::cout << "IRISSlicer<itk>: "; tp.Start();
    typedef IRISSlicer<Seg3DImageType, Seg2DImageType, Seg3DImageType> slicerTypeITK;
    slicerTypeITK::Pointer roiITK = slicerTypeITK::New();
    roiITK->SetInput(itkImage);
    roiITK->SetSliceIndex(sliceIndex);
    roiITK->SetSliceDirectionImageAxis(sliceAxis);
    roiITK->SetLineDirectionImageAxis(lineAxis);
    roiITK->SetPixelDirectionImageAxis(pixelAxis);
    roiITK->SetLineTraverseForward(lineForward);
    roiITK->SetPixelTraverseForward(pixelForward);
    roiITK->Update();
    Seg2DImageType::Pointer sliceITK = roiITK->GetOutput();
    tp.Stop(); std::cout << tp.GetMean() * 1000 << " ms " << std::endl; tp.Reset();

    // Now compare the two images
    typedef itk::Testing::ComparisonImageFilter< Seg2DImageType, Seg2DImageType > DiffType;
    DiffType::Pointer diff = DiffType::New();
    diff->SetValidInput(sliceITK);
    diff->SetTestInput(sliceRLE);
    diff->UpdateLargestPossibleRegion();
    std::cout << "Number of pixels with difference: " << 
        diff->GetNumberOfPixelsWithDifferences() << std::endl << std::endl;
}

//test all 4 combinations of bool parameters (lineForward and pixelForward)
void test4bools(shortRLEImage::Pointer rleImage, Seg3DImageType::Pointer itkImage,
    unsigned sliceIndex, unsigned sliceAxis, unsigned lineAxis, unsigned pixelAxis)
{
    testIRISSlicer(rleImage, itkImage, sliceIndex, sliceAxis, lineAxis, pixelAxis, true, true);
    testIRISSlicer(rleImage, itkImage, sliceIndex, sliceAxis, lineAxis, pixelAxis, true, false);
    testIRISSlicer(rleImage, itkImage, sliceIndex, sliceAxis, lineAxis, pixelAxis, false, true);
    testIRISSlicer(rleImage, itkImage, sliceIndex, sliceAxis, lineAxis, pixelAxis, false, false);
}

int main(int argc, char* argv[])
{
    itk::TimeProbe tp;
    std::cout << "Loading image: "; tp.Start();
    Seg3DImageType::Pointer inImage = loadImage(argv[1]);
    tp.Stop(); std::cout << tp.GetMean() * 1000 << " ms " << std::endl; tp.Reset();
        
    std::cout << "itk->RLE conversion: "; tp.Start();
    shortRLEImage::Pointer test = shortRLEImage::New();
    typedef itk::RegionOfInterestImageFilter<Seg3DImageType, shortRLEImage> inConverterType;
    inConverterType::Pointer inConv = inConverterType::New();
    inConv->SetInput(inImage);
    inConv->SetRegionOfInterest(inImage->GetLargestPossibleRegion());
    inConv->Update();
    test = inConv->GetOutput();
    tp.Stop(); std::cout << tp.GetMean() * 1000 << " ms " << std::endl; tp.Reset();

    //Test all 6 permutations of axes
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(2) / 2, 2, 1, 0);
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(2) / 2, 2, 0, 1);
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(1) / 2, 1, 2, 0);
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(1) / 2, 1, 0, 2);
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(0) / 2, 0, 2, 1);
    test4bools(test, inImage, inImage->GetBufferedRegion().GetSize(0) / 2, 0, 1, 2);
    std::cout << "All tests finished!";
    getchar();
}
