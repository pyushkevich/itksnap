#include <iostream>
#include <stdexcept>
#include <utility>

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
#include "IRISSlicer.h"
#include "RLERegionOfInterestImageFilter.h"
#include <itkTimeProbe.h>

typedef itk::Image<short, 3> Seg3DImageType;
typedef itk::Image<short, 2> Seg2DImageType;
typedef itk::LabelMap<itk::LabelObject<short, 3 > > Label3DType;
typedef itk::LabelMap<itk::LabelObject<short, 2 > > Label2DType;

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

typedef RLEImage<short> RLEImage3D;
typedef std::pair<short, short> RLSegment;
typedef std::vector<RLSegment> RLLine;
typedef std::vector<std::vector<RLLine> > RLImage;

RLImage toRLImage(Seg3DImageType::Pointer image)
{
    itk::Size<3> iSize = image->GetLargestPossibleRegion().GetSize();
    RLImage result(iSize[2]);
    for (int z = 0; z < iSize[2]; z++)
        result[z].resize(iSize[1]);
    itk::Index<3> ind;
    for (int z = 0; z < iSize[2]; z++)
    {
        ind[2] = z;
        for (int y = 0; y < iSize[1]; y++)
        {
            ind[1] = y;
            ind[0] = 0;
            int x = 0;
            RLLine l;         
            while (x < iSize[0])
            {
                RLSegment s(0, image->GetPixel(ind));
                while (x < iSize[0] && image->GetPixel(ind) == s.second)
                {
                    x++;
                    s.first++;
                    ind[0] = x;
                }
                l.push_back(s);
            }
            result[z][y] = l;
        }
    }
    return result;
}

int sliceIndex, axis;
Seg3DImageType::RegionType reg;

void uncompressLine(RLLine line, short *out)
{
    for (int x = 0; x < line.size(); x++)
        for (int r = 0; r < line[x].first; r++)
            *(out++) = line[x].second;
}

void cropRLI(RLImage image, short *outSlice)
{
    itk::Size<3> size;
    size[2] = image.size();
    size[1] = image[0].size();
    size[0] = 0;
    for (int x = 0; x < image[0][0].size(); x++)
        size[0] += image[0][0][x].first;

    if (axis == 2) //slicing along z
        for (int y = 0; y < size[1]; y++)
            uncompressLine(image[sliceIndex][y], outSlice + y*size[0]);
    else if (axis == 1) //slicing along y
        for (int z = 0; z < size[2]; z++)
            uncompressLine(image[z][sliceIndex], outSlice + z*size[0]);
    else //slicing along x, the low-preformance case
        for (int z = 0; z < size[2]; z++)
            for (int y = 0; y < size[1]; y++)
            {
                int t = 0;
                for (int x = 0; x < image[z][y].size(); x++)
                {
                    t += image[z][y][x].first;
                    if (t > sliceIndex)
                    {
                        *(outSlice++) = image[z][y][x].second;
                        break;
                    }
                }
            }
}

Seg3DImageType::Pointer cropNormal(Seg3DImageType::Pointer image)
{
    typedef itk::RegionOfInterestImageFilter<Seg3DImageType, Seg3DImageType> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(image);
    roi->SetRegionOfInterest(reg);
    roi->Update();
    return roi->GetOutput();
}

Seg2DImageType::Pointer cropIRIS(Seg3DImageType::Pointer image)
{
    typedef IRISSlicer<Seg3DImageType, Seg2DImageType, Seg3DImageType> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(image);
    roi->SetSliceIndex(sliceIndex);
    roi->SetSliceDirectionImageAxis(axis);
    if (axis == 0) //x
    {
        roi->SetLineDirectionImageAxis(2);
        roi->SetPixelDirectionImageAxis(1);
    }
    else if (axis==1) //y
    {
        roi->SetLineDirectionImageAxis(2);
        roi->SetPixelDirectionImageAxis(0);
    }
    else //z
    {
        roi->SetLineDirectionImageAxis(1);
        roi->SetPixelDirectionImageAxis(0);
    }
    roi->Update();
    return roi->GetOutput();
}

Seg2DImageType::Pointer cropRLEiris(RLEImage3D::Pointer image)
{
    typedef IRISSlicer<RLEImage3D, Seg2DImageType, RLEImage3D> roiType;
    roiType::Pointer roi = roiType::New();
    roi->SetInput(image);
    roi->SetSliceIndex(sliceIndex);
    roi->SetSliceDirectionImageAxis(axis);
    if (axis == 0) //x
    {
        roi->SetLineDirectionImageAxis(2);
        roi->SetPixelDirectionImageAxis(1);
    }
    else if (axis == 1) //y
    {
        roi->SetLineDirectionImageAxis(2);
        roi->SetPixelDirectionImageAxis(0);
    }
    else //z
    {
        roi->SetLineDirectionImageAxis(1);
        roi->SetPixelDirectionImageAxis(0);
    }
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

    //below conversion has easily predictable and low computational complexity
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
        cout << "Usage:\n" << argv[0] << " InputImage3D.ext OutputSlice2D.ext X|Y|Z SliceNumber [RLE|RLI|IRIS|Normal]" << endl;
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
    default: throw std::runtime_error("Axis should be X, Y or Z");
    }

    bool rle = true;
    if (argc>5)
        if (strcmp(argv[5], "RLE") && strcmp(argv[5], "rle"))
            rle = false;
    bool rli = false;
    if (argc>5)
        if (strcmp(argv[5], "RLI") == 0 || strcmp(argv[5], "rli") == 0)
            rli = true;
    bool iris = false;
    if (argc>5)
        if (strcmp(argv[5], "IRIS") == 0 || strcmp(argv[5], "iris") == 0)
            iris = true;
    bool irisRLE = false;
    if (argc>5)
        if (strcmp(argv[5], "irisRLE") == 0 || strcmp(argv[5], "irisrle") == 0)
            irisRLE = true;
    bool memCheck = false;
    if (argc>6)
        if (strcmp(argv[6], "MEM") == 0 || strcmp(argv[6], "mem") == 0)
            memCheck = true;

    Seg3DImageType::Pointer cropped, inImage = loadImage(argv[1]);
    Label3DType::Pointer inLabelMap;
    RLEImage3D::Pointer rleImage;
    RLImage rlImage;
    reg = inImage->GetLargestPossibleRegion();
    reg.SetIndex(axis, sliceIndex);
    reg.SetSize(axis, 1);
    Seg2DImageType::Pointer cropped2D = cropIRIS(inImage); //pre-allocate slice

    if (rle)
    {
        inLabelMap = toLabelMap(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image
    }
    if (rli)
    {
        rlImage = toRLImage(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image
        memset(cropped2D->GetBufferPointer(), 0, sizeof(short)*cropped2D->GetOffsetTable()[2]); //clear buffer
    }
    if (irisRLE)
    {
        typedef itk::RegionOfInterestImageFilter<Seg3DImageType, RLEImage3D> inConverterType;
        inConverterType::Pointer inConv = inConverterType::New();
        inConv->SetInput(inImage);
        inConv->SetRegionOfInterest(inImage->GetLargestPossibleRegion());
        inConv->Update();
        rleImage = inConv->GetOutput();
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
    else if (rli)
        cropRLI(rlImage, cropped2D->GetBufferPointer());
    else if (iris)
        cropped2D = cropIRIS(inImage);
    else if (irisRLE)
        cropped2D = cropRLEiris(rleImage);
    else
        cropped = cropNormal(inImage);
    tp.Stop();

    if (rle)
        cout << "RLE";
    else if (rli)
        cout << "RLI";
    else if (iris)
        cout << "IRIS";
    else if (irisRLE)
        cout << "irisRLE";
    else
        cout << "Normal";

    cout << " slicing took: " << tp.GetMean() * 1000 << " ms " << endl;


    if (!iris && !rli && !irisRLE)
    {
        typedef itk::ExtractImageFilter<Seg3DImageType, Seg2DImageType> eiType;
        eiType::Pointer ei = eiType::New();
        ei->SetDirectionCollapseToIdentity();
        ei->SetInput(cropped);
        reg.SetIndex(axis, 0);
        cropped->SetRegions(reg);
        reg.SetSize(axis, 0);
        ei->SetExtractionRegion(reg);
        ei->Update();
        cropped2D = ei->GetOutput();
    }

    SegWriterType::Pointer wr = SegWriterType::New();
    wr->SetInput(cropped2D);
    wr->SetFileName(argv[2]);
    wr->SetUseCompression(true);
    wr->Update();
    return 0;
}
