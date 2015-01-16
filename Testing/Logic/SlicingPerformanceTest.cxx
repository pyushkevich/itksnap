#include <iostream>
#include <stdexcept>

using namespace std;

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkLabelMap.h>
#include <itkLabelObject.h>
#include <itkLabelMapToLabelImageFilter.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkChangeRegionLabelMapFilter.h>
#include <itkTestingComparisonImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkTimeProbe.h>

typedef itk::Image<unsigned short, 3> Seg3DImageType;
typedef itk::Image<unsigned short, 2> Seg2DImageType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 3 > > Label3DType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 2 > > Label2DType;

typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
typedef itk::ImageFileWriter<Seg2DImageType> SegWriterType;

Seg3DImageType::Pointer loadImage(const string filename)
{
    SegReaderType::Pointer sr = SegReaderType::New();
    sr->SetFileName(filename);
    sr->Update();
    return sr->GetOutput();
}

Label3DType::Pointer toLabelMap(Seg3DImageType::Pointer image)
{
    typedef itk::LabelImageToLabelMapFilter<Seg3DImageType> i2lType;
    i2lType::Pointer i2l = i2lType::New();
    i2l->SetInput(image);
    i2l->Update();
    return i2l->GetOutput();
}

int sliceIndex, axis;
Seg3DImageType::RegionType reg;

Seg3DImageType::Pointer cropNormal(Seg3DImageType::Pointer image)
{
    typedef itk::RegionOfInterestImageFilter<Seg3DImageType, Seg3DImageType> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(image);
    roi->SetRegionOfInterest(reg);
    roi->Update();
    return roi->GetOutput();
}

Seg3DImageType::Pointer cropRLE(Label3DType::Pointer image)
{
    typedef itk::ChangeRegionLabelMapFilter<Label3DType> roiLMType;
    roiLMType::Pointer roiLM = roiLMType::New();
    roiLM->SetInput(image);
    roiLM->SetRegion(reg);
    roiLM->Update();

    //below conversion has low computational complexity
    typedef itk::LabelMapToLabelImageFilter<Label3DType, Seg3DImageType> lm2liType;
    lm2liType::Pointer lm2li = lm2liType::New();
    lm2li->SetInput(roiLM->GetOutput());
    lm2li->Update();
    return lm2li->GetOutput();
}

//do some slicing operations, measure time taken
int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        cout << "Usage:\n" << argv[0] << " InputImage3D.ext OutputSlice2D.ext X|Y|Z SliceNumber [RLE|Normal] [MEM]" << endl;
        return 1;
    }

    sliceIndex = atoi(argv[4]);
    switch (argv[3][0])
    {
    case 'X':
    case 'x': axis = 0; break;
    case 'Y':
    case 'y': axis = 1; break;
    case 'Z':
    case 'z': axis = 2; break;
    default: throw std::runtime_error("Axis should be X, Y or Z"); break;
    }

    bool rle = true;
    if (argc>5)
        if (strcmp(argv[5], "RLE") && strcmp(argv[5], "rle"))
            rle = false;
    bool memCheck = false;
    if (argc>6)
        if (strcmp(argv[6], "MEM")==0 || strcmp(argv[6], "mem")==0)
            memCheck = true;

    Seg3DImageType::Pointer cropped, inImage = loadImage(argv[1]);
    Label3DType::Pointer inLabelMap;
    reg = inImage->GetLargestPossibleRegion();
    reg.SetIndex(axis, sliceIndex);
    reg.SetSize(axis, 1);
    if (rle)
    {
        inLabelMap = toLabelMap(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image
    }

    if (memCheck)
    {
        cout << "Now check memory consumption";
        getchar();
    }
    itk::TimeProbe tp;
    tp.Start();
    if (rle)
        cropped = cropRLE(inLabelMap);
    else
        cropped = cropNormal(inImage);
    tp.Stop();

    cout << (rle ? "RLE" : "Normal") << " slicing took: " << tp.GetMean() * 1000 << " ms " << endl;


    typedef itk::ExtractImageFilter<Seg3DImageType, Seg2DImageType> eiType;
    eiType::Pointer ei = eiType::New();
    ei->SetDirectionCollapseToIdentity();
    ei->SetInput(cropped);
    reg.SetIndex(axis, 0);
    cropped->SetRegions(reg);
    reg.SetSize(axis, 0);
    ei->SetExtractionRegion(reg);
    ei->Update();

    SegWriterType::Pointer wr = SegWriterType::New();
    wr->SetInput(ei->GetOutput());
    wr->SetFileName(argv[2]);
    wr->SetUseCompression(true);
    wr->Update();
    return 0;
}