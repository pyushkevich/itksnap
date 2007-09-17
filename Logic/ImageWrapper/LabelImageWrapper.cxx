/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LabelImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2007/09/17 14:22:02 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "ImageWrapper.txx"
#include "ScalarImageWrapper.txx"
#include "LabelImageWrapper.h"
#include "ColorLabel.h"
#include "ColorLabelTable.h"

// Create an instance of ImageWrapper of appropriate type
template class ImageWrapper<LabelType>;
template class ScalarImageWrapper<LabelType>;

LabelImageWrapper
::LabelImageWrapper()
{
  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_RGBAFilter[i] = RGBAFilterType::New();
    m_RGBAFilter[i]->SetInput(m_Slicer[i]->GetOutput());
  }

  SetLabelColorTable(NULL);
}

LabelImageWrapper
::LabelImageWrapper(const LabelImageWrapper &source)
: ScalarImageWrapper<LabelType>(source)
{
  // Instantiate the filters
  for(unsigned int i=0;i<3;i++) 
  {
    m_RGBAFilter[i] = RGBAFilterType::New();
    m_RGBAFilter[i]->SetInput(m_Slicer[i]->GetOutput());
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
  return m_RGBAFilter[0]->GetColorTable();
}

void 
LabelImageWrapper
::SetLabelColorTable(ColorLabelTable *table) 
{
  // Set the new table
  for(unsigned int i=0;i<3;i++) 
    m_RGBAFilter[i]->SetColorTable(table);
}

void 
LabelImageWrapper
::UpdateColorMappingCache() 
{
  // Better have a color table
  assert(GetLabelColorTable());

  // Dirty the intensity filters
  for(unsigned int i=0;i<3;i++)
    m_RGBAFilter[i]->Modified();  
}

LabelImageWrapper::DisplaySliceType *
LabelImageWrapper
::GetDisplaySlice(unsigned int dim)
{
  return m_RGBAFilter[dim]->GetOutput();
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
