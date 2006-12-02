/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LabelImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:13 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "ImageWrapper.h"
#include "ImageWrapper.txx"
#include "LabelImageWrapper.h"
#include "ColorLabel.h"
#include "ColorLabelTable.h"

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<LabelType>;

LabelImageWrapper
::LabelImageWrapper()
{
  // Instantiate the cache
  m_IntensityMapCache = CacheType::New();

  // Set the target of the cache
  m_IntensityMapCache->SetInputFunctor(&m_IntensityFunctor);

  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetFunctor(m_IntensityMapCache->GetCachingFunctor());
    m_IntensityFilter[i]->SetInput(m_Slicer[i]->GetOutput());
  }

  // Initialize the color table as well
  m_IntensityFunctor.m_ColorLabelTable = NULL;
}

LabelImageWrapper
::LabelImageWrapper(const LabelImageWrapper &source)
: ImageWrapper<LabelType>(source)
{
  // Instantiate the cache
  m_IntensityMapCache = CacheType::New();

  // Set the target of the cache
  m_IntensityMapCache->SetInputFunctor(&m_IntensityFunctor);

  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetFunctor(m_IntensityMapCache->GetCachingFunctor());
    m_IntensityFilter[i]->SetInput(m_Slicer[i]->GetOutput());
  }

  // Initialize the color table as well
  SetLabelColorTable(source.GetLabelColorTable());
}

LabelImageWrapper
::~LabelImageWrapper()
{
}

ColorLabelTable *
LabelImageWrapper
::GetLabelColorTable() const
{
  return m_IntensityFunctor.m_ColorLabelTable;
}

void 
LabelImageWrapper
::SetLabelColorTable(ColorLabelTable *table) 
{
  // Set the new table
  m_IntensityFunctor.m_ColorLabelTable = table;

  // Reinitialize the cache
  // TODO: Constant for 255
  m_IntensityMapCache->SetEvaluationRange(0,255);

  // Update the color mapping cache and the filters
  UpdateColorMappingCache();
}

void 
LabelImageWrapper
::UpdateColorMappingCache() 
{
  assert(m_IntensityFunctor.m_ColorLabelTable);

  // Update the label table
  m_IntensityMapCache->ComputeCache();

  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_IntensityFilter[i]->Modified();  
}

LabelImageWrapper::DisplaySliceType *
LabelImageWrapper
::GetDisplaySlice(unsigned int dim)
{
  return m_IntensityFilter[dim]->GetOutput();
}

LabelImageWrapper::DisplayPixelType
LabelImageWrapper::IntensityFunctor
::operator()(const LabelType &x) const
{
  // Better have the table!
  assert(m_ColorLabelTable);

  // Get the appropriate color label
  const ColorLabel &label = m_ColorLabelTable->GetColorLabel(x);

  // Create a new pixel
  DisplayPixelType pixel;

  // Figure out the alpha for display
  unsigned char alpha = 
    label.IsVisible() ? (label.IsOpaque() ? 255 : label.GetAlpha()) : 0;

  // Copy the color and transparency attributes
  // pixel.Set(label.GetRGB(0),label.GetRGB(1),label.GetRGB(2),alpha);
  pixel[0] = label.GetRGB(0);
  pixel[1] = label.GetRGB(1);
  pixel[2] = label.GetRGB(2);
  pixel[3] = alpha;

  return pixel;
}

/**
 * This definition is needed to use RGBA pixels for compilation
 */
typedef itk::RGBAPixel<unsigned char> ColorPixel;

namespace itk {

template<>
class NumericTraits<ColorPixel>
{
public:
  typedef ColorPixel ValueType;
  typedef ColorPixel PrintType;
  typedef ColorPixel AbsType;
  typedef ColorPixel AccumulateType;
  static const ColorPixel Zero;
  static const ColorPixel One;

  static ColorPixel NonpositiveMin() { return Zero; }
  static bool IsPositive(ColorPixel val) { return true; }
  static bool IsNonpositive(ColorPixel val) { return false; }
  static bool IsNegative(ColorPixel val) { return false; }
  static bool IsNonnegative(ColorPixel val) {return true; }
private:

  static const unsigned char ZeroArray[4];
  static const unsigned char OneArray[4];
};

} // End of namespace

const unsigned char itk::NumericTraits<ColorPixel>::ZeroArray[4] = {0,0,0,0};
const ColorPixel itk::NumericTraits<ColorPixel>::Zero = 
  ColorPixel(itk::NumericTraits<ColorPixel>::ZeroArray);

const unsigned char itk::NumericTraits<ColorPixel>::OneArray[4] = {1,1,1,1};
const ColorPixel itk::NumericTraits<ColorPixel>::One = 
  ColorPixel(itk::NumericTraits<ColorPixel>::OneArray);
