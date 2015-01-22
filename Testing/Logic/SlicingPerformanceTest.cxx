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

typedef std::pair<short, short> RLSegment;
typedef std::vector<RLSegment> RLLine;
typedef std::vector<std::vector<RLLine> > RLImage; //RLE along z axis

RLImage toRLImage(Seg3DImageType::Pointer image)
{
    itk::Size<3> iSize = image->GetLargestPossibleRegion().GetSize();
    RLImage result(iSize[0]);
    for (int i = 0; i < iSize[0]; i++)
        result[i].resize(iSize[1]);
    itk::Index<3> ind;
    for (int i = 0; i < iSize[0]; i++)
    {
        ind[0] = i;
        for (int j = 0; j < iSize[1]; j++)
        {
            ind[1] = j;
            ind[2] = 0;
            int k = 0;
            RLLine l;         
            while (k < iSize[2])
            {
                RLSegment s(0, image->GetPixel(ind));
                while (k < iSize[2] && image->GetPixel(ind) == s.second)
                {
                    k++;
                    s.first++;
                    ind[2] = k;
                }
                l.push_back(s);
            }
            result[i][j] = l;
        }
    }
    return result;
}

int sliceIndex, axis;
Seg3DImageType::RegionType reg;

inline std::vector<short> uncompressLine(RLLine line, int len)
{
    std::vector<short> res(len);
    int i = 0;
    for (int k = 0; k < line.size(); k++)
        for (int r = 0; r < line[k].first; r++)
            res[i++] = line[k].second;
    return res;
}

std::vector<std::vector<short> > cropRLI(RLImage image)
{
    itk::Size<3> size;
    size[0] = image.size();
    size[1] = image[0].size();
    size[2] = 0;
    for (int k = 0; k < image[0][0].size(); k++)
        size[2] += image[0][0][k].first;

    std::vector<std::vector<short> > result;
    if (axis == 0) //slicing along x
    {
        result.resize(size[1]);
        for (int j = 0; j < size[1]; j++)
            result[j] = uncompressLine(image[sliceIndex][j], size[2]);
    }
    else if (axis == 1) //slicing along y
    {
        result.resize(size[0]);
        for (int i = 0; i < size[0]; i++)
            result[i] = uncompressLine(image[i][sliceIndex], size[2]);
    }
    else //slicing along z, the low-preformance case
    {
        result.resize(size[0]);
        for (int i = 0; i < size[0]; i++)
        {
            result[i].resize(size[1]);
            for (int j = 0; j < size[1]; j++)
            {
                int t = 0;
                for (int k = 0; k < image[i][j].size(); k++)
                {
                    t += image[i][j][k].first;
                    if (t >= sliceIndex)
                    {
                        result[i][j] = image[i][j][k].second;
                        break;
                    }
                }
            }
        }
    }

    return result;
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
        cout << "Usage:\n" << argv[0] << " InputImage3D.ext OutputSlice2D.ext X|Y|Z SliceNumber [RLE|RLI|Normal]" << endl;
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
    bool memCheck = false;
    if (argc>6)
        if (strcmp(argv[6], "MEM") == 0 || strcmp(argv[6], "mem") == 0)
            memCheck = true;

    Seg3DImageType::Pointer cropped, inImage = loadImage(argv[1]);
    Label3DType::Pointer inLabelMap;
    RLImage rlImage;
    reg = inImage->GetLargestPossibleRegion();
    reg.SetIndex(axis, sliceIndex);
    reg.SetSize(axis, 1);
    if (rle)
    {
        inLabelMap = toLabelMap(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image
    }
    if (rli)
    {
        rlImage = toRLImage(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image
    }
    if (memCheck)
    {
        cout << "Now check memory consumption";
        getchar();
    }

    std::vector<std::vector<short> > rlS;
    itk::TimeProbe tp;
    tp.Start();
    if (rle)
        cropped = cropRLE(inLabelMap);
    else if (rli)
        rlS = cropRLI(rlImage);
    else
        cropped = cropNormal(inImage);
    tp.Stop();

    if (rle)
        cout << "RLE";
    else if (rli)
        cout << "RLI";
    else
        cout << "Normal";

    cout << " slicing took: " << tp.GetMean() * 1000 << " ms " << endl;

    if (rli) //now convert rlS into cropped
    {
        //first make cropped the normal way
        inImage = loadImage(argv[1]);
        cropped = cropNormal(inImage);
        inImage = Seg3DImageType::New(); //effectively deletes the image

        memset(cropped->GetBufferPointer(), 0, sizeof(short)*cropped->GetOffsetTable()[3]); //clear buffer
        //now overwrite it with data from rlS
        short *p = cropped->GetBufferPointer();
        for (int i = 0; i < rlS.size(); i++)
        {
            memcpy(p, &rlS[i][0], sizeof(short)*rlS[i].size());
            p += rlS[i].size();
        }
    }

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