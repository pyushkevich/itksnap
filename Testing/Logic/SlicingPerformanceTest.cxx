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
#include <itkMemoryProbe.h>

typedef itk::Image<unsigned short, 3> Seg3DImageType;
typedef itk::Image<unsigned short, 2> Seg2DImageType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 3 > > Label3DType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 2 > > Label2DType;

typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
typedef itk::ImageFileWriter<Seg2DImageType> SegWriterType;

//do some slicing operations, measure time taken
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "Usage:\n" << argv[0] << " InputImage3D.ext OutputSlice2D.ext X|Y|Z SliceNumber" << endl;
        return 1;
    }

    SegReaderType::Pointer sr = SegReaderType::New();
    sr->SetFileName(argv[1]);
    sr->Update();
    Seg3DImageType::Pointer croppedI, inImage = sr->GetOutput();

    typedef itk::LabelImageToLabelMapFilter<Seg3DImageType> i2lType;
    i2lType::Pointer i2l = i2lType::New();
    i2l->SetInput(inImage);
    i2l->Update();
    Label3DType::Pointer croppedLM, inLabelMap = i2l->GetOutput();

    int slice = atoi(argv[4]);
    int axis;
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
    Seg3DImageType::RegionType reg = inImage->GetLargestPossibleRegion();
    reg.SetIndex(axis, slice);
    reg.SetSize(axis, 1);

    itk::TimeProbe tp0, tp1, tp2;
    itk::MemoryProbe mp0, mp1, mp2;
    
    mp0.Start();
    tp0.Start();
    typedef itk::RegionOfInterestImageFilter<Seg3DImageType, Seg3DImageType> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(inImage);
    roi->SetRegionOfInterest(reg);
    roi->Update();
    croppedI = roi->GetOutput();
    tp0.Stop();
    mp0.Stop();

    mp1.Start();
    tp1.Start();
    typedef itk::ChangeRegionLabelMapFilter<Label3DType> roiLMType;
    roiLMType::Pointer roiLM = roiLMType::New();
    roiLM->SetInput(inLabelMap);
    roiLM->SetRegion(reg);
    roiLM->Update();
    croppedLM = roiLM->GetOutput();
    tp1.Stop();
    mp1.Stop();

    mp2.Start();
    tp2.Start();
    typedef itk::LabelMapToLabelImageFilter<Label3DType, Seg3DImageType> lm2liType;
    lm2liType::Pointer lm2li = lm2liType::New();
    lm2li->SetInput(croppedLM);
    lm2li->Update();
    Seg3DImageType::Pointer croppedL = lm2li->GetOutput();
    tp2.Stop();
    mp2.Stop();

    cout << "ImageSlicing: " << tp0.GetMean() * 1000 << " ms & " << mp0.GetMean() << mp0.GetUnit() << endl;
    cout << "LabelMapSlicing: " << tp1.GetMean() * 1000 << " ms & " << mp0.GetMean() << mp0.GetUnit() << endl;
    cout << "LabelMap to LabelImage conversion: " << tp2.GetMean() * 1000
        << " ms & " << mp0.GetMean() << mp0.GetUnit() << endl;

    typedef itk::ExtractImageFilter<Seg3DImageType, Seg2DImageType> eiType;
    eiType::Pointer ei = eiType::New();
    ei->SetDirectionCollapseToIdentity();
    ei->SetInput(croppedL);
    //ei->SetInput(croppedI);
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
