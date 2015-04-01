#include "MomentTextures.h"
#include "itkImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkNeighborhoodIterator.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

const int MAX_VAL=100000;

namespace bilwaj {

//Returns the estimated moment around the mean associated of the degree(th) order
//TO DO Ext defaults to 1; Ideally should be part of moments class
void MomentTexture(SmartPtr<ImageType> &Inp, SmartPtr<ImageType> &MomentTexture, int degree, int Ext)
{
  IteratorType TexIt(MomentTexture,MomentTexture->GetBufferedRegion());
  typedef itk::NeighborhoodIterator<ImageType> NeighborhoodIterator;
  NeighborhoodIterator::RadiusType radius;
  for(int i=0;i<ImageType::ImageDimension;i++)
    {radius[i]=1;}
  NeighborhoodIterator InpIt(radius,Inp,Inp->GetBufferedRegion());
  TexIt.GoToBegin();
  while(!TexIt.IsAtEnd())
    {
    float accum=0.0;
    float min=0; //Store minimum voxel intensity value
    float max=0; //Store maximum voxel intensity value

    //first compute the mean
    //TO DO use mean texture function to not recompute the mean every time
    for(int j=0;j<InpIt.Size();j++)
      {
      accum=accum+InpIt.GetPixel(j);
      min=MIN(min,InpIt.GetPixel(j));
      max=MAX(max,InpIt.GetPixel(j));
      }
    float range=max-min;
    float mean=accum/InpIt.Size();
    double accumX=0.0;
    for(int j=0;j<InpIt.Size();j++)
      {
      //Modified expression for moments to keep them range bound and manageable
      accumX=accumX+std::pow((InpIt.GetPixel(j)-mean)/range,degree);
      }
    //100 is usedas a multiplier to keep images produced to be visualized
    TexIt.Set((datatype)(1000*accumX/InpIt.Size()));
    ++TexIt;
    ++InpIt;
    }
  std::cout<<"Traversing done"<<std::endl;
}

}
/*
int main(int argc, char *argv[])
{
  ReaderType::Pointer input = ReaderType::New();
  //Input file
  input->SetFileName("1.nii");
  input->Update();

  ImageType::Pointer Inp=ImageType::New();
  Inp=input->GetOutput();

  ImageType::Pointer Tex = ImageType::New();;
  Tex->CopyInformation(Inp);
  Tex->SetRequestedRegion( Inp->GetRequestedRegion() );
  Tex->SetBufferedRegion( Inp->GetBufferedRegion() );
  Tex->Allocate();
  Tex->FillBuffer(0);


  //Second moment neighborhood radius of 3
  MomentTexture(Inp,Tex,2,3);
  //The function is generic and multiple runs ,ay be used to create feature vectors as desired Inp and Tex are image types


  WriterType::Pointer output = WriterType::New();
  //Output file
  output->SetFileName("out.nii");
  output->SetInput(Tex);
  output->Update();

  //std::cout<<max_val<<std::endl;
  return EXIT_SUCCESS;
}
*/
