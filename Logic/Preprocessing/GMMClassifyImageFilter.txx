#ifndef GMMCLASSIFYIMAGEFILTER_TXX
#define GMMCLASSIFYIMAGEFILTER_TXX

#include "GMMClassifyImageFilter.h"
#include "itkImageRegionConstIterator.h"

template <class TInputImage, class TOutputImage>
GMMClassifyImageFilter<TInputImage, TOutputImage>
::GMMClassifyImageFilter()
{
  m_MixtureModel = NULL;
}

template <class TInputImage, class TOutputImage>
GMMClassifyImageFilter<TInputImage, TOutputImage>
::~GMMClassifyImageFilter()
{
}

template <class TInputImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TOutputImage>
::SetMixtureModel(GaussianMixtureModel *model)
{
  m_MixtureModel = model;
  this->Modified();
}

template <class TInputImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  os << indent << "GMMClassifyImageFilter" << std::endl;
}

template <class TInputImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  // Get the number of inputs
  assert(m_MixtureModel);
  int n_input = this->GetNumberOfIndexedInputs();
  OutputImagePointer outputPtr = this->GetOutput(0);

  // Define n image iterators
  typedef itk::ImageRegionConstIterator<TInputImage> InputIter;
  InputIter *it_in = new InputIter[n_input];
  int *comps = new int[n_input];
  double *x = new double[m_MixtureModel->GetNumberOfComponents()];
  for(int i = 0; i < n_input; i++)
    {
    const InputImageType *input = this->GetInput(i);
    InputIter test(input, outputRegionForThread);
    it_in[i] = test;
    comps[i] = input->GetNumberOfComponentsPerPixel();
    }


  typedef itk::ImageRegionIterator<TOutputImage> OutputIter;
  OutputIter it_out(outputPtr, outputRegionForThread);

  // Iterate through all the voxels
  while ( !it_out.IsAtEnd() )
    {
    // Build up a vector of all the component values (x)
    int iComp = 0;
    for(int i = 0; i < n_input; i++)
      {
      InputPixelType pix = it_in[i].Get();
      for(int j = 0; j < comps[i]; j++, iComp++)
        {
        x[iComp] = pix[j];
        }
      ++it_in[i];
      }

    // Evaluate the GMM for each of the clusters
    double p = 0.0, psum = 0;
    for(int k = 0; k < m_MixtureModel->GetNumberOfGaussians(); k++)
      {
      // TODO: this should be the foreground component
      double q = m_MixtureModel->EvaluatePDF(k, x) * m_MixtureModel->GetWeight(k);
      psum += q;
      if(k == 0)
        p = q;
      //else
      //  p -= q;
      }

    if(it_out.GetIndex()[0] == 37 &&
       it_out.GetIndex()[1] == 13 &&
       it_out.GetIndex()[2] == 4)
      std::cout << "Test posterior = " << p/psum << std::endl;

    double post = psum == 0 ? 0.0 : p / psum;


    // Store the value
    it_out.Set(post * 0x7fff);
    ++it_out;
    }
}

#endif
