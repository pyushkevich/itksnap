#ifndef __itkTopologyPreservingDigitalSurfaceEvolutionImageFilter_txx
#define __itkTopologyPreservingDigitalSurfaceEvolutionImageFilter_txx

#include "itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.h"

#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryDiamondStructuringElement.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkSubtractImageFilter.h"
#include "itkTimeProbe.h"

namespace itk
{

template<class TImage>
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::TopologyPreservingDigitalSurfaceEvolutionImageFilter()
{
  this->SetNumberOfRequiredInputs( 1 );
  this->m_NumberOfIterations = 5;
  this->m_ThresholdValue = 0.5;
  this->m_BackgroundValue = NumericTraits<PixelType>::Zero;
  this->m_ForegroundValue = NumericTraits<PixelType>::One;
  this->m_UseInversionMode = false;
  this->m_TargetImage = NULL;

  if( ImageDimension == 2 )
    {
    this->InitializeIndices2D();
    }
  else if( ImageDimension == 3 )
    {
    this->InitializeIndices3D();
    }
  else
    {
    itkExceptionMacro( "Image dimension must be equal to 2 or 3." );
    }
}

template<class TImage>
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::~TopologyPreservingDigitalSurfaceEvolutionImageFilter()
{
}

template<class TImage>
void
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::GenerateData()
{
  this->AllocateOutputs();

  if( !this->m_TargetImage )
    {
    itkExceptionMacro( "TargetImage not specified." );
    }

  RealType totalDifference = 0.0;

  ImageRegionIterator<ImageType> It( this->GetOutput(),
    this->GetOutput()->GetRequestedRegion() );
  ImageRegionIterator<ImageType> ItT( this->m_TargetImage,
    this->m_TargetImage->GetRequestedRegion() );
  for( It.GoToBegin(), ItT.GoToBegin(); !It.IsAtEnd(); ++It, ++ItT )
    {
    if( It.Get() != ItT.Get() )
      {
      totalDifference += 1.0;
      }
    if( this->m_UseInversionMode )
      {
      if( It.Get() == this->m_ForegroundValue )
        {
        It.Set( this->m_BackgroundValue );
        }
      else
        {
        It.Set( this->m_ForegroundValue );
        }
      if( ItT.Get() == this->m_ForegroundValue )
        {
        ItT.Set( this->m_BackgroundValue );
        }
      else
        {
        ItT.Set( this->m_ForegroundValue );
        }
      }
    }

  this->m_SurfaceLabel = this->m_ForegroundValue + 1;

  unsigned int iterations = 0;
  bool changeDetected = true;

  RealType totalNumberOfChanges = 0.0;
//  std::cout << "/" << std::flush;
//
//  TimeProbe timer;
//  timer.Start();

  this->UpdateProgress( 0.0 );
//  RealType oldProgress = this->GetProgress() * 100.0;

  while( iterations++ < this->m_NumberOfIterations && changeDetected )
    {
    changeDetected = false;

    this->CreateLabelSurfaceImage();

    ImageRegionIteratorWithIndex<ImageType> ItI( this->GetOutput(),
      this->GetOutput()->GetRequestedRegion() );

    ImageRegionIterator<ImageType> ItT( this->m_TargetImage,
      this->m_TargetImage->GetRequestedRegion() );
    ImageRegionIterator<ImageType> ItL( this->m_LabelSurfaceImage,
      this->m_LabelSurfaceImage->GetRequestedRegion() );

    ItI.GoToBegin();
    ItT.GoToBegin();
    ItL.GoToBegin();

    while( !ItI.IsAtEnd() )
      {
      RealType absoluteDifference
        = vnl_math_abs( static_cast<RealType>( ItT.Get() - ItI.Get() ) );

      if( ItL.Get() == this->m_SurfaceLabel )
        {
        if( absoluteDifference > this->m_ThresholdValue )
          {
          bool isChangeSafe = false;
          if( ImageDimension == 2 )
            {
            isChangeSafe = this->IsChangeSafe2D( ItI.GetIndex() );
            }
          else
            {
            isChangeSafe = this->IsChangeSafe3D( ItI.GetIndex() );
            }
          if( isChangeSafe )
            {
            ItI.Set( this->m_ForegroundValue );
            changeDetected = true;

            totalNumberOfChanges += 1.0;
            this->UpdateProgress( totalNumberOfChanges / totalDifference );
//            RealType newProgress = this->GetProgress() * 100.0;
//
//            if( newProgress - oldProgress >= 1.0 )
//              {
//              oldProgress = newProgress;
//              std::cout << "*" << std::flush;
//              }
            }
          }
        }
      ++ItI;
      ++ItT;
      ++ItL;
      }
    }
//  timer.Stop();
//  std::cout << "/ -> " << this->GetProgress() << " ("
//    << timer.GetMeanTime() << " seconds)" << std::endl;


  if( this->m_UseInversionMode )
    {
    ImageRegionIterator<ImageType> It( this->GetOutput(),
      this->GetOutput()->GetRequestedRegion() );
    ImageRegionIterator<ImageType> ItT( this->m_TargetImage,
      this->m_TargetImage->GetRequestedRegion() );
    for( It.GoToBegin(), ItT.GoToBegin(); !It.IsAtEnd(); ++It, ++ItT )
      {
      if( It.Get() == this->m_ForegroundValue )
        {
        It.Set( this->m_BackgroundValue );
        }
      else
        {
        It.Set( this->m_ForegroundValue );
        }
      if( ItT.Get() == this->m_ForegroundValue )
        {
        ItT.Set( this->m_BackgroundValue );
        }
      else
        {
        ItT.Set( this->m_ForegroundValue );
        }
      }
    }

  this->UpdateProgress( 1.0 );
}

template<class TImage>
void
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::CreateLabelSurfaceImage()
{
  typedef BinaryDiamondStructuringElement<PixelType,
    ImageDimension> StructuringElementType;
  StructuringElementType element;
  element.SetRadius( 1 );
  element.CreateStructuringElement();

  typedef BinaryDilateImageFilter<ImageType, ImageType,
    StructuringElementType> DilateFilterType;
  typename DilateFilterType::Pointer dilater = DilateFilterType::New();
  dilater->SetKernel( element );
  dilater->SetInput( this->GetOutput() );
  dilater->SetBackgroundValue( this->m_BackgroundValue );
  dilater->SetForegroundValue( this->m_ForegroundValue );
  dilater->Update();

  typedef SubtractImageFilter
    <ImageType, ImageType, ImageType> SubtracterType;
  typename SubtracterType::Pointer subtracter = SubtracterType::New();
  subtracter->SetInput1( dilater->GetOutput() );
  subtracter->SetInput2( this->GetOutput() );
  subtracter->Update();

  typedef BinaryThresholdImageFilter<ImageType, ImageType> ThresholderType;

  typename ThresholderType::Pointer thresholder = ThresholderType::New();

  thresholder->SetInput( subtracter->GetOutput() );

  thresholder->SetLowerThreshold( this->m_ForegroundValue );

  thresholder->SetUpperThreshold( this->m_ForegroundValue );

  thresholder->SetInsideValue( this->m_SurfaceLabel );

  thresholder->SetOutsideValue( this->m_BackgroundValue );

  thresholder->Update();

  this->m_LabelSurfaceImage = thresholder->GetOutput();
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsChangeSafe2D( IndexType idx )
{
  typename NeighborhoodIteratorType::RadiusType radius;
  radius.Fill( 1 );
  NeighborhoodIteratorType It( radius, this->GetOutput(),
        this->GetOutput()->GetRequestedRegion() );
  It.SetLocation( idx );

  Array<short> neighborhoodPixels( 9 );

  // Check for critical configurations: 4 90-degree rotations

  for ( unsigned int i = 0; i < 4; i++ )
    {
    for ( unsigned int j = 0; j < 9; j++ )
      {
      neighborhoodPixels[j] =
        ( It.GetPixel( this->m_RotationIndices[i][j] )
        == this->m_BackgroundValue );
      if( this->m_RotationIndices[i][j] == 4 )
        {
        neighborhoodPixels[j] = !neighborhoodPixels[j];
        }
      }

    if( this->IsCriticalC1Configuration2D( neighborhoodPixels )
      || this->IsCriticalC2Configuration2D( neighborhoodPixels )
      || this->IsCriticalC3Configuration2D( neighborhoodPixels )
      || this->IsCriticalC4Configuration2D( neighborhoodPixels ) )
      {
      return false;
      }
    }

  // Check for critical configurations: 2 reflections
  //  Note that the reflections for the C1 and C2 cases
  //  are covered by the rotation cases above (except
  //  in the case of FullInvariance == false.

  for ( unsigned int i = 0; i < 2; i++ )
    {
    for ( unsigned int j = 0; j < 9; j++ )
      {
      neighborhoodPixels[j] =
        ( It.GetPixel( this->m_ReflectionIndices[i][j] )
        == this->m_BackgroundValue );
      if( this->m_ReflectionIndices[i][j] == 4 )
        {
        neighborhoodPixels[j] = !neighborhoodPixels[j];
        }
      }
//    if( !this->m_FullInvariance
//      && ( this->IsCriticalC1Configuration2D( neighborhoodPixels )
//        || this->IsCriticalC2Configuration2D( neighborhoodPixels ) ) )
//      {
//      return false;
//      }
    if( this->IsCriticalC3Configuration2D( neighborhoodPixels )
      || this->IsCriticalC4Configuration2D( neighborhoodPixels ) )
      {
      return false;
      }
    }

  /**
   * this check is only valid after the other checks have
   * been performed.
   */
  if( this->IsCriticalTopologicalConfiguration( idx ) )
    {
    return false;
    }

  return true;

}

template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalTopologicalConfiguration( IndexType idx )
{
  typename NeighborhoodIteratorType::RadiusType radius;
  radius.Fill( 1 );
  NeighborhoodIteratorType It( radius, this->GetOutput(),
        this->GetOutput()->GetRequestedRegion() );
  It.SetLocation( idx );

  unsigned int numberOfCriticalC3Configurations = 0;
  unsigned int numberOfFaces = 0;
  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    if( It.GetNext( d ) == this->m_ForegroundValue )
      {
      numberOfFaces++;
      }
    if( It.GetPrevious( d ) == this->m_ForegroundValue )
      {
      numberOfFaces++;
      }
    if( It.GetNext( d ) == this->m_ForegroundValue
      && It.GetPrevious( d ) == this->m_ForegroundValue )
      {
      numberOfCriticalC3Configurations++;
      }
    }

  if( numberOfCriticalC3Configurations > 0 && numberOfFaces % 2 == 0
      && numberOfCriticalC3Configurations * 2 == numberOfFaces )
    {
    return true;
    }
  return false;
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC1Configuration2D( Array<short> neighborhood )
{
  return ( !neighborhood[0] &&  neighborhood[1] &&
            neighborhood[3] && !neighborhood[4] &&
           !neighborhood[8] );
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC2Configuration2D( Array<short> neighborhood )
{
  return ( !neighborhood[0] &&  neighborhood[1] &&
            neighborhood[3] && !neighborhood[4] &&
            neighborhood[8] &&
           ( neighborhood[5] || neighborhood[7] ) );
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC3Configuration2D( Array<short> neighborhood )
{
  return ( !neighborhood[0] &&  neighborhood[1] &&
            neighborhood[3] && !neighborhood[4] &&
           !neighborhood[5] &&  neighborhood[6] &&
           !neighborhood[7] &&  neighborhood[8] );
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC4Configuration2D( Array<short> neighborhood )
{
  return ( !neighborhood[0] &&  neighborhood[1] &&
            neighborhood[3] && !neighborhood[4] &&
           !neighborhood[5] && !neighborhood[6] &&
           !neighborhood[7] &&  neighborhood[8] );
}

/*
 * 2-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsSpecialCaseOfC4Configuration2D( PixelType label, IndexType idx,
                                    IndexType idx6, IndexType idx7 )
{
  IndexType idxa;
  IndexType idxb;
  for ( unsigned int j = 0; j < 2; j++ )
    {
    idxa[j] = idx7[j] + ( idx7[j] - idx[j] );
    idxb[j] = idx6[j] + ( idx7[j] - idx[j] );
    }
  return ( this->GetOutput()->GetRequestedRegion().IsInside( idxa ) &&
           this->GetOutput()->GetRequestedRegion().IsInside( idxb ) &&
           this->GetOutput()->GetPixel( idxa ) <  label &&
           this->GetOutput()->GetPixel( idxb ) >= label );
}

/*
 * 2-D
 */
template<class TImage>
void
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::InitializeIndices2D()
{
  this->m_RotationIndices[0].SetSize( 9 );
  this->m_RotationIndices[1].SetSize( 9 );
  this->m_RotationIndices[2].SetSize( 9 );
  this->m_RotationIndices[3].SetSize( 9 );

  this->m_RotationIndices[0][0] = 0;
  this->m_RotationIndices[0][1] = 1;
  this->m_RotationIndices[0][2] = 2;
  this->m_RotationIndices[0][3] = 3;
  this->m_RotationIndices[0][4] = 4;
  this->m_RotationIndices[0][5] = 5;
  this->m_RotationIndices[0][6] = 6;
  this->m_RotationIndices[0][7] = 7;
  this->m_RotationIndices[0][8] = 8;

  this->m_RotationIndices[1][0] = 2;
  this->m_RotationIndices[1][1] = 5;
  this->m_RotationIndices[1][2] = 8;
  this->m_RotationIndices[1][3] = 1;
  this->m_RotationIndices[1][4] = 4;
  this->m_RotationIndices[1][5] = 7;
  this->m_RotationIndices[1][6] = 0;
  this->m_RotationIndices[1][7] = 3;
  this->m_RotationIndices[1][8] = 6;

  this->m_RotationIndices[2][0] = 8;
  this->m_RotationIndices[2][1] = 7;
  this->m_RotationIndices[2][2] = 6;
  this->m_RotationIndices[2][3] = 5;
  this->m_RotationIndices[2][4] = 4;
  this->m_RotationIndices[2][5] = 3;
  this->m_RotationIndices[2][6] = 2;
  this->m_RotationIndices[2][7] = 1;
  this->m_RotationIndices[2][8] = 0;

  this->m_RotationIndices[3][0] = 6;
  this->m_RotationIndices[3][1] = 3;
  this->m_RotationIndices[3][2] = 0;
  this->m_RotationIndices[3][3] = 7;
  this->m_RotationIndices[3][4] = 4;
  this->m_RotationIndices[3][5] = 1;
  this->m_RotationIndices[3][6] = 8;
  this->m_RotationIndices[3][7] = 5;
  this->m_RotationIndices[3][8] = 2;

  this->m_ReflectionIndices[0].SetSize( 9 );
  this->m_ReflectionIndices[1].SetSize( 9 );

  this->m_ReflectionIndices[0][0] = 6;
  this->m_ReflectionIndices[0][1] = 7;
  this->m_ReflectionIndices[0][2] = 8;
  this->m_ReflectionIndices[0][3] = 3;
  this->m_ReflectionIndices[0][4] = 4;
  this->m_ReflectionIndices[0][5] = 5;
  this->m_ReflectionIndices[0][6] = 0;
  this->m_ReflectionIndices[0][7] = 1;
  this->m_ReflectionIndices[0][8] = 2;

  this->m_ReflectionIndices[1][0] = 2;
  this->m_ReflectionIndices[1][1] = 1;
  this->m_ReflectionIndices[1][2] = 0;
  this->m_ReflectionIndices[1][3] = 5;
  this->m_ReflectionIndices[1][4] = 4;
  this->m_ReflectionIndices[1][5] = 3;
  this->m_ReflectionIndices[1][6] = 8;
  this->m_ReflectionIndices[1][7] = 7;
  this->m_ReflectionIndices[1][8] = 6;
}

/*
 * 3-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsChangeSafe3D( IndexType idx )
{
  Array<short> neighborhoodPixels( 8 );

  typename NeighborhoodIteratorType::RadiusType radius;
  radius.Fill( 1 );
  NeighborhoodIteratorType It( radius, this->GetOutput(),
        this->GetOutput()->GetRequestedRegion() );
  It.SetLocation( idx );

  // Check for C1 critical configurations
  for ( unsigned int i = 0; i < 12; i++ )
    {
    for ( unsigned int j = 0; j < 4; j++ )
      {
      neighborhoodPixels[j]
        = ( It.GetPixel( this->m_C1Indices[i][j] ) == this->m_ForegroundValue );
      if( this->m_C1Indices[i][j] == 13 )
        {
        neighborhoodPixels[j] = !neighborhoodPixels[j];
        }
      }
    if( this->IsCriticalC1Configuration3D( neighborhoodPixels ) )
      {
      return false;
      }
    }

  // Check for C2 critical configurations
  for ( unsigned int i = 0; i < 8; i++ )
    {
    for ( unsigned int j = 0; j < 8; j++ )
      {
      neighborhoodPixels[j]
        = ( It.GetPixel( this->m_C2Indices[i][j] ) == this->m_ForegroundValue );
      if( this->m_C2Indices[i][j] == 13 )
        {
        neighborhoodPixels[j] = !neighborhoodPixels[j];
        }
      }
    if( this->IsCriticalC2Configuration3D( neighborhoodPixels ) )
      {
      return false;
      }
    }

  /**
   * this check is only valid after the other checks have
   * been performed.
   */
  if( this->IsCriticalTopologicalConfiguration( idx ) )
    {
    return false;
    }

  return true;
}


/*
 * 3-D
 */
template<class TImage>
bool
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC1Configuration3D( Array<short> neighborhood )
{
  return ( (  neighborhood[0] &&  neighborhood[1] &&
             !neighborhood[2] && !neighborhood[3] ) ||
           ( !neighborhood[0] && !neighborhood[1] &&
              neighborhood[2] &&  neighborhood[3] ) );
}

/*
 * 3-D
 */
template<class TImage>
unsigned int
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::IsCriticalC2Configuration3D( Array<short> neighborhood )
{
  // Check if Type 1 or Type 2
  for ( unsigned int i = 0; i < 4; i++ )
    {
    bool isC2 = false;
    if( neighborhood[2*i] == neighborhood[2*i+1] )
      {
      isC2 = true;
      for ( unsigned int j = 0; j < 8; j++ )
        {
        if( neighborhood[j] == neighborhood[2*i] &&
               j != 2*i && j != 2*i+1 )
          {
          isC2 = false;
          }
        }
      }
    if( isC2 )
      {
      if( neighborhood[2*i] )
        {
        return 1;
        }
      else
        {
        return 2;
        }
      }
    }

  return 0;
}

/*
 * 3-D
 */
template<class TImage>
void
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::InitializeIndices3D()
{
  for ( unsigned int i = 0; i <  12; i++ )
    {
    this->m_C1Indices[i].SetSize( 4 );
    }
  for ( unsigned int i = 0; i <  8; i++ )
    {
    this->m_C2Indices[i].SetSize( 8 );
    }

  this->m_C1Indices[0][0] = 1;
  this->m_C1Indices[0][1] = 13;
  this->m_C1Indices[0][2] = 4;
  this->m_C1Indices[0][3] = 10;

  this->m_C1Indices[1][0] = 9;
  this->m_C1Indices[1][1] = 13;
  this->m_C1Indices[1][2] = 10;
  this->m_C1Indices[1][3] = 12;

  this->m_C1Indices[2][0] = 3;
  this->m_C1Indices[2][1] = 13;
  this->m_C1Indices[2][2] = 4;
  this->m_C1Indices[2][3] = 12;

  this->m_C1Indices[3][0] = 4;
  this->m_C1Indices[3][1] = 14;
  this->m_C1Indices[3][2] = 5;
  this->m_C1Indices[3][3] = 13;

  this->m_C1Indices[4][0] = 12;
  this->m_C1Indices[4][1] = 22;
  this->m_C1Indices[4][2] = 13;
  this->m_C1Indices[4][3] = 21;

  this->m_C1Indices[5][0] = 13;
  this->m_C1Indices[5][1] = 23;
  this->m_C1Indices[5][2] = 14;
  this->m_C1Indices[5][3] = 22;

  this->m_C1Indices[6][0] = 4;
  this->m_C1Indices[6][1] = 16;
  this->m_C1Indices[6][2] = 7;
  this->m_C1Indices[6][3] = 13;

  this->m_C1Indices[7][0] = 13;
  this->m_C1Indices[7][1] = 25;
  this->m_C1Indices[7][2] = 16;
  this->m_C1Indices[7][3] = 22;

  this->m_C1Indices[8][0] = 10;
  this->m_C1Indices[8][1] = 22;
  this->m_C1Indices[8][2] = 13;
  this->m_C1Indices[8][3] = 19;

  this->m_C1Indices[9][0] = 12;
  this->m_C1Indices[9][1] = 16;
  this->m_C1Indices[9][2] = 13;
  this->m_C1Indices[9][3] = 15;

  this->m_C1Indices[10][0] = 13;
  this->m_C1Indices[10][1] = 17;
  this->m_C1Indices[10][2] = 14;
  this->m_C1Indices[10][3] = 16;

  this->m_C1Indices[11][0] = 10;
  this->m_C1Indices[11][1] = 14;
  this->m_C1Indices[11][2] = 11;
  this->m_C1Indices[11][3] = 13;

  this->m_C2Indices[0][0] = 0;
  this->m_C2Indices[0][1] = 13;
  this->m_C2Indices[0][2] = 1;
  this->m_C2Indices[0][3] = 12;
  this->m_C2Indices[0][4] = 3;
  this->m_C2Indices[0][5] = 10;
  this->m_C2Indices[0][6] = 4;
  this->m_C2Indices[0][7] = 9;

  this->m_C2Indices[4][0] = 9;
  this->m_C2Indices[4][1] = 22;
  this->m_C2Indices[4][2] = 10;
  this->m_C2Indices[4][3] = 21;
  this->m_C2Indices[4][4] = 12;
  this->m_C2Indices[4][5] = 19;
  this->m_C2Indices[4][6] = 13;
  this->m_C2Indices[4][7] = 18;

  for ( unsigned int i = 1; i < 4; i++ )
    {
    int addend;
    if( i == 2 )
      {
      addend = 2;
      }
    else
      {
      addend = 1;
      }
    for ( unsigned int j = 0; j < 8; j++ )
      {
      this->m_C2Indices[i  ][j] = this->m_C2Indices[i-1][j] + addend;
      this->m_C2Indices[i+4][j] = this->m_C2Indices[i+3][j] + addend;
      }
    }

}

template <class TImage>
void
TopologyPreservingDigitalSurfaceEvolutionImageFilter<TImage>
::PrintSelf(
  std::ostream& os,
  Indent indent) const
{
  Superclass::PrintSelf( os, indent );
}

} // end namespace itk

#endif

