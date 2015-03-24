#include "RLEImageRegionIterator.h"
#include "RLERegionOfInterestImageFilter.h"
#include <iostream>
#include <string>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkTimeProbe.h>

using namespace std;

typedef itk::Image<short, 3> Seg3DImageType;
typedef itk::Image<short, 2> Seg2DImageType;

typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
typedef itk::ImageFileWriter<Seg3DImageType> SegWriterType;

Seg3DImageType::Pointer loadImage(const std::string filename)
{
    SegReaderType::Pointer sr = SegReaderType::New();
    sr->SetFileName(filename);
    sr->Update();
    return sr->GetOutput();
}

void writeImage(Seg3DImageType::Pointer image, const std::string filename, bool compress = true)
{
    SegWriterType::Pointer sw = SegWriterType::New();
    sw->SetInput(image);
    sw->SetFileName(filename);
    sw->SetUseCompression(compress);
    sw->Update();
}

int main(int argc, char* argv[])
{
    typedef RLEImage<short> shortRLEImage;
    itk::TimeProbe tp;
    std::cout << "Loading image: "; tp.Start();
    Seg3DImageType::Pointer inImage = loadImage(argv[1]);
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();
    std::cout << "itk->RLE conversion: "; tp.Start();
    shortRLEImage::Pointer test = shortRLEImage::New();
    test->fromITKImage(inImage);
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();
    test->Print(std::cout);

    cout << "Invoking RegionOfInterest filter:"; tp.Start();
    shortRLEImage::RegionType r = test->GetLargestPossibleRegion();
    for (int i = 0; i < 3; i++)
    {
        r.SetIndex(i, r.GetIndex(i) + r.GetSize(i) / 4);
        r.SetSize(i, r.GetSize(i) / 2);
    }
    typedef itk::RegionOfInterestImageFilter<shortRLEImage, shortRLEImage> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(test);
    roi->SetRegionOfInterest(r);
    //roi->SetNumberOfThreads(1);
    roi->Update();
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();
    
    cout << "Assignment operator:"; tp.Start();
    test = roi->GetOutput();
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();

    std::cout << "RLE->itk conversion: "; tp.Start();
    inImage = test->toITKImage();
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();
    std::cout << " writing image" << endl;
    writeImage(inImage, "test.mha", true);
}