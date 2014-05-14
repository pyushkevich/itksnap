#ifndef GMMCLASSIFYIMAGEFILTER_TXX
#define GMMCLASSIFYIMAGEFILTER_TXX

#include "GMMClassifyImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "EMGaussianMixtures.h"
#include "ImageCollectionToImageFilter.h"

template <class TInputImage, class TInputVectorImage, class TOutputImage>
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::GMMClassifyImageFilter()
{
  m_MixtureModel = NULL;
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::~GMMClassifyImageFilter()
{
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::SetMixtureModel(GaussianMixtureModel *model)
{
  m_MixtureModel = model;
  this->Modified();
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::AddScalarImage(InputImageType *image)
{
  this->AddInput(image);
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::AddVectorImage(InputVectorImageType *image)
{
  this->AddInput(image);
}


template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  itk::ImageSource<TOutputImage>::GenerateInputRequestedRegion();

  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    {
    // Check whether the input is an image of the appropriate dimension
    InputImageType *input = dynamic_cast< InputImageType * >( it.GetInput() );
    InputVectorImageType *vecInput = dynamic_cast< InputVectorImageType * >( it.GetInput() );
    if (input)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      input->SetRequestedRegion(inputRegion);
      }
    else if(vecInput)
      {
      InputImageRegionType inputRegion;
      this->CallCopyOutputRegionToInputRegion( inputRegion, this->GetOutput()->GetRequestedRegion() );
      vecInput->SetRequestedRegion(inputRegion);
      }
    }
}


template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  os << indent << "GMMClassifyImageFilter" << std::endl;
}

template <class TInputImage, class TInputVectorImage, class TOutputImage>
void
GMMClassifyImageFilter<TInputImage, TInputVectorImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  // Get the number of inputs
  assert(m_MixtureModel);
  int n_input = this->GetNumberOfIndexedInputs();
  OutputImagePointer outputPtr = this->GetOutput(0);

  // Create a collection iterator
  typedef ImageCollectionConstRegionIteratorWithIndex<
      TInputImage, TInputVectorImage> CollectionIter;

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

  // Configure the input collection iterator
  CollectionIter cit(outputRegionForThread);
  for( itk::InputDataObjectIterator it( this ); !it.IsAtEnd(); it++ )
    cit.AddImage(it.GetInput());

  // Get the number of components
  int nComp = cit.GetTotalComponents();

  // Iterate through all the voxels
  while ( !it_out.IsAtEnd() )
    {
    for(int i = 0; i < nComp; i++)
      {
      x[i] = cit.Value(i);
      }

    // Evaluate the posterior probability robustly
    for(int k = 0; k < m_MixtureModel->GetNumberOfGaussians(); k++)
      {
      log_pdf[k] = m_MixtureModel->EvaluateLogPDF(k, x, x_scratch);
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
    ++cit;
    }
}


#endif
