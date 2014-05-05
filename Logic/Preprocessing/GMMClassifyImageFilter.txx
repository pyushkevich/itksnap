#ifndef GMMCLASSIFYIMAGEFILTER_TXX
#define GMMCLASSIFYIMAGEFILTER_TXX

#include "GMMClassifyImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "EMGaussianMixtures.h"

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

/*
template <class TInputImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
  this->AllocateOutputs();

  // Get the number of inputs
  assert(m_MixtureModel);
  int n_input = this->GetNumberOfIndexedInputs();
  OutputImagePointer outputPtr = this->GetOutput(0);

  // Define n image iterators
  typedef itk::ImageRegionConstIterator<TInputImage> InputIter;
  InputIter *it_in = new InputIter[n_input];
  int *comps = new int[n_input];
  for(int i = 0; i < n_input; i++)
    {
    const InputImageType *input = this->GetInput(i);
    InputIter test(input, outputPtr->GetRequestedRegion());
    it_in[i] = test;
    comps[i] = input->GetNumberOfComponentsPerPixel();
    }


  typedef itk::ImageRegionIterator<TOutputImage> OutputIter;
  OutputIter it_out(outputPtr, outputPtr->GetRequestedRegion());

  vnl_vector<double> x(m_MixtureModel->GetNumberOfComponents());
  vnl_vector<double> x_scratch(m_MixtureModel->GetNumberOfComponents());
  vnl_vector<double> p(m_MixtureModel->GetNumberOfGaussians());


  // Iterate through all the voxels
  while ( !it_out.IsAtEnd() )
    {
    bool debug =false;
    if(it_out.GetIndex()[0] == 30 && it_out.GetIndex()[1] == 22 && it_out.GetIndex()[2] == 7)
      debug = true;

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

    double psum = 0;
    for(int k = 0; k < m_MixtureModel->GetNumberOfGaussians(); k++)
      {
      // TODO: this should be the foreground component
      p[k] = m_MixtureModel->EvaluatePDF(k, x, x_scratch) * m_MixtureModel->GetWeight(k);
      psum += p[k];
      }


    double post = (psum == 0) ? 0.0 : p[2] / psum;


    // Store the value
    it_out.Set(post * 0x7fff);
    ++it_out;
    }
}
*/

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
  for(int i = 0; i < n_input; i++)
    {
    const InputImageType *input = this->GetInput(i);
    InputIter test(input, outputRegionForThread);
    it_in[i] = test;
    comps[i] = input->GetNumberOfComponentsPerPixel();
    }


  typedef itk::ImageRegionIterator<TOutputImage> OutputIter;
  OutputIter it_out(outputPtr, outputRegionForThread);

  vnl_vector<double> x(m_MixtureModel->GetNumberOfComponents());
  vnl_vector<double> x_scratch(m_MixtureModel->GetNumberOfComponents());
  vnl_vector<double> z_scratch(m_MixtureModel->GetNumberOfComponents());
  vnl_vector<double> log_pdf(m_MixtureModel->GetNumberOfGaussians());
  vnl_vector<double> log_w(m_MixtureModel->GetNumberOfGaussians());
  vnl_vector<double> w(m_MixtureModel->GetNumberOfGaussians());
  vnl_vector<double> p(m_MixtureModel->GetNumberOfGaussians());

  // Create a multiplier vector (1 for foreground, -1 for background)
  vnl_vector<double> pfactor(m_MixtureModel->GetNumberOfGaussians());
  for(int i = 0; i < m_MixtureModel->GetNumberOfGaussians(); i++)
    {
    pfactor[i] = m_MixtureModel->IsForeground(i) ? 1.0 : -1.0;
    log_w[i] = log(m_MixtureModel->GetWeight(i));
    w[i] = m_MixtureModel->GetWeight(i);
    }

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

    // Evaluate the posterior probability robustly
    for(int k = 0; k < m_MixtureModel->GetNumberOfGaussians(); k++)
      {
      log_pdf[k] = m_MixtureModel->EvaluateLogPDF(k, x, x_scratch, z_scratch);
      }

    // Evaluate the GMM for each of the clusters
    double pdiff = 0;
    for(int k = 0; k < m_MixtureModel->GetNumberOfGaussians(); k++)
      {
      p[k] = EMGaussianMixtures::ComputePosterior(
            m_MixtureModel->GetNumberOfGaussians(),
            log_pdf.data_block(), w.data_block(), log_w.data_block(), k);

      pdiff += p[k] * pfactor[k];
      }

    // Store the value
    it_out.Set((OutputPixelType)(pdiff * 0x7fff));
    ++it_out;
    }
}


#endif
