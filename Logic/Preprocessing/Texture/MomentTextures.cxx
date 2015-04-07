#include "MomentTextures.h"
#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkNeighborhoodIterator.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

const int MAX_VAL=100000;

namespace bilwaj {

template <class TInputImage, class TOutputImage>
void
MomentTextureFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const RegionType & outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  // Iterator for the output region
  typedef itk::ImageRegionIteratorWithIndex<OutputImageType> OutputIteratorType;
  OutputIteratorType TexIt(this->GetOutput(), outputRegionForThread);

  // Neighborhood iterator for the input region
  typedef itk::ConstNeighborhoodIterator<InputImageType> NeighborhoodIterator;
  NeighborhoodIterator InpIt(m_Radius,this->GetInput(), outputRegionForThread);

  // Accumulator array
  vnl_vector<float> accumX(m_HighestDegree);
  OutputPixelType out_pix(m_HighestDegree);

  for( TexIt.GoToBegin(); !TexIt.IsAtEnd(); ++TexIt, ++InpIt)
    {
    // On the first pass through the neighborhood compute the mean intensity and
    // the intensity range in the neighborhood
    float accum=0.0;
    float min=0; //Store minimum voxel intensity value
    float max=0; //Store maximum voxel intensity value

    for(int j=0;j<InpIt.Size();j++)
      {
      InputPixelType pix = InpIt.GetPixel(j);
      accum += pix;
      min=MIN(min, pix);
      max=MAX(max, pix);
      }

    float range = max-min;
    float mean = accum / InpIt.Size();

    // On the second pass, compute the moments
    accumX.fill(0.0f);
    for(int j=0;j<InpIt.Size();j++)
      {
      InputPixelType pix = InpIt.GetPixel(j);
      float norm_val = ((pix - mean) / range), norm_val_k = norm_val;
      accumX[0] += norm_val;
      for(int k = 1; k < m_HighestDegree; k++)
        {
        norm_val_k *= norm_val;
        accumX[k] += norm_val_k;
        }
      }

    // Scale the moments by neighborhood size
    accumX /= InpIt.Size();

    // The first moment should just be the mean
    accumX[0] = mean / range;

    // Assign to the output voxel
    for(int k = 0; k < m_HighestDegree; k++)
      {
      out_pix[k] = static_cast<OutputComponentType>(1000 * accumX[k]);
      }

    // Assign to the output voxel
    TexIt.Set(out_pix);
    }
}

template <class TInputImage, class TOutputImage>
void
MomentTextureFilter<TInputImage, TOutputImage>
::UpdateOutputInformation()
{
  Superclass::UpdateOutputInformation();
  this->GetOutput()->SetNumberOfComponentsPerPixel(m_HighestDegree);
}

template class MomentTextureFilter<itk::Image<short, 3>, itk::VectorImage<short, 3> >;

/*
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
*/

} // namespace


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
