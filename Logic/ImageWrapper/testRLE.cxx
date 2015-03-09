#include "RLEImage.h"
#include "RLEImageRegionConstIterator.h"
#include "RLEImageRegionIterator.h"
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

void writeImage(Seg3DImageType::Pointer image, const std::string filename)
{
    SegWriterType::Pointer sw = SegWriterType::New();
    sw->SetInput(image);
    sw->SetFileName(filename);
    sw->SetUseCompression(true);
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

    typedef itk::ImageRegionIterator<shortRLEImage> itType;
    itType itNoI(test, test->GetLargestPossibleRegion());
    typedef itk::ImageRegionIteratorWithIndex<shortRLEImage> itTypeWI;
    itTypeWI itWI(test, test->GetLargestPossibleRegion());

    auto it = itWI;
    it.GoToEnd();
    if (it.IsAtEnd())
        it.GoToBegin();
    while (!it.IsAtEnd())
    {
        cout << it.GetIndex() << ": " << it.Value() << endl;
        ++it;
    }

    cout << "Reverse iteration:" << endl;

    do
    {
        --it; //not working properly
        cout << it.GetIndex() << ": " << it.Value() << endl;
    } while (!it.IsAtBegin());

    std::cout << "Allocating RLEimage: "; tp.Start();
    test->Allocate();
    tp.Stop(); cout << tp.GetMean() << " us " << endl; tp.Reset();
    std::cout << "Filling buffer: "; tp.Start();
    test->FillBuffer(1983);
    tp.Stop(); cout << tp.GetMean() << " us " << endl; tp.Reset();
    shortRLEImage::IndexType ind;
    ind[0] = 1;
    ind[1] = 2;
    ind[2] = 1;
    std::cout << "Setting a pixel: "; tp.Start();
    test->SetPixel(ind, 100);
    tp.Stop(); cout << tp.GetMean() << " us " << endl; tp.Reset();
    std::cout << "Getting a pixel: "; tp.Start();
    short val = test->GetPixel(ind);
    tp.Stop(); cout << tp.GetMean() << " us " << endl; tp.Reset();
    test->Print(std::cout);

    std::cout << "RLE->itk conversion: "; tp.Start();
    inImage = test->toITKImage();
    tp.Stop(); cout << tp.GetMean() * 1000 << " ms " << endl; tp.Reset();
    std::cout << " writing image" << endl;
    writeImage(inImage, "test.mha");
}