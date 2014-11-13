#include <iostream>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::chrono;

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

typedef itk::Image<unsigned short, 3> Seg3DImageType;
typedef itk::Image<unsigned short, 2> Seg2DImageType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 3 > > Label3DType;
typedef itk::LabelMap<itk::LabelObject<unsigned short, 2 > > Label2DType;

typedef itk::ImageFileReader<Seg3DImageType> SegReaderType;
typedef itk::ImageFileWriter<Seg2DImageType> SegWriterType;

//do some slicing operations, measure time taken
int main(int argc, char *argv[])
{
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
  default: throw std::exception("Axis should be X, Y or Z"); break;
  }
  Seg3DImageType::RegionType reg = inImage->GetLargestPossibleRegion();
  reg.SetIndex(axis, slice);
  reg.SetSize(axis, 1);

  auto start = steady_clock::now();
  
  typedef itk::RegionOfInterestImageFilter<Seg3DImageType, Seg3DImageType> roiType;
  roiType::Pointer roi = roiType::New();
  roi->SetInput(inImage);
  roi->SetRegionOfInterest(reg);
  roi->Update();
  croppedI = roi->GetOutput();

  auto end1 = steady_clock::now();

  typedef itk::ChangeRegionLabelMapFilter<Label3DType> roiLMType;
  roiLMType::Pointer roiLM = roiLMType::New();
  roiLM->SetInput(inLabelMap);
  roiLM->SetRegion(reg);
  roiLM->Update();
  croppedLM = roiLM->GetOutput();

  auto end2 = steady_clock::now();

  typedef itk::LabelMapToLabelImageFilter<Label3DType, Seg3DImageType> lm2liType;
  lm2liType::Pointer lm2li = lm2liType::New();
  lm2li->SetInput(croppedLM);
  lm2li->Update();
  Seg3DImageType::Pointer croppedL = lm2li->GetOutput();

  auto end3 = steady_clock::now();
  auto end4 = steady_clock::now();

  cout << "ImageSlicing: " << chrono::duration <double, milli>(end1 - start).count() << " ms" << endl;
  cout << "LabelMapSlicing: " << chrono::duration <double, milli>(end2 - end1).count() << " ms" << endl;
  cout << "LabelMap to LabelImage conversion: " << chrono::duration <double, milli>(end3 - end2).count() << " ms" << endl;

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
  wr->Update();
  return 0;
}

