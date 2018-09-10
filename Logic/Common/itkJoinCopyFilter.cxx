////image filter to set DrawingColor in output image if input pixel equals SeedValue

#ifndef __itkJoinCopyFilter_cxx
#define __itkJoinCopyFilter_cxx
 
#include "itkJoinCopyFilter.h"
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkProgressReporter.h>

namespace itk{

    template<typename TInputImage1, typename TInputImage2>
    JoinCopyFilter<TInputImage1, TInputImage2>
    ::JoinCopyFilter(){
	m_SeedActive= false;
	m_SeedIndex.Fill(0);
	m_SeedValue= NumericTraits<typename TInputImage2::PixelType>::Zero;
	}

    template<typename TInputImage1, typename TInputImage2>
    void JoinCopyFilter<TInputImage1, TInputImage2>
    ::SetJsrc(const TInputImage1 *image1){
	this->SetNthInput( 0, const_cast< TInputImage1 * >( image1 ) );
	}

    template<typename TInputImage1, typename TInputImage2>
    void JoinCopyFilter<TInputImage1, TInputImage2>
    ::SetJdst(const TInputImage2 *image2){
	this->SetNthInput( 1, const_cast< TInputImage2 * >( image2 ) );
	}


    template<typename TInputImage1, typename TInputImage2>
    void JoinCopyFilter<TInputImage1, TInputImage2>
    ::BeforeThreadedGenerateData(){

	if(m_SeedActive){
	    typename TInputImage1::ConstPointer input = dynamic_cast<const TInputImage1 *>( ProcessObject::GetInput(0) );
	    m_SeedValue= input->GetPixel(m_SeedIndex);
	    }
	m_UpdateFlag= false;
	}


    template<typename TInputImage1, typename TInputImage2>
    void JoinCopyFilter<TInputImage1, TInputImage2>
    ::ThreadedGenerateData(const typename Superclass::OutputImageRegionType& outputRegionForThread, ThreadIdType threadId){

	if(m_SeedActive){
	    typename TInputImage1::ConstPointer input = dynamic_cast<const TInputImage1 *>( ProcessObject::GetInput(0) );
	    typename TInputImage2::Pointer output = this->GetOutput();
 
	    itk::ImageRegionConstIterator<TInputImage1> iti(input, outputRegionForThread);
	    itk::ImageRegionIterator<TInputImage2> ito(output, outputRegionForThread);
 
	    // support progress methods/callbacks with 1000 updates
	    ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels(), 1000);

	    while(!iti.IsAtEnd()){
		if(iti.Get() == m_SeedValue){
		    ito.Set(m_DrawingColor);
		    }
		++iti;
		++ito;
		progress.CompletedPixel();
		}

	    m_UpdateFlag= true;
	    m_SeedActive= false;
	    //output->Modified();
	    }
	}
    }// end namespace
 
#endif //__itkJoinCopyFilter_cxx
