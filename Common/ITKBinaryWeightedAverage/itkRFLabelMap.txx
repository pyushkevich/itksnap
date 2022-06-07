#ifndef itkRFLabelMap_txx
#define itkRFLabelMap_txx

#include "itkObjectFactory.h"
//#include "itkRFLabelMap.h"

namespace itk
{

template< class TInputImage>
void
RFLabelMap<TInputImage>
::GenerateData()
{

    typename TInputImage::ConstPointer input = this->GetInput();
    typename TInputImage::SizeType radius;

    std::vector<int> dimensions;
    dimensions.push_back(0);
    dimensions.push_back(1);
    dimensions.push_back(2);

    dimensions.erase(dimensions.begin() + m_SlicingAxis);

    int DIL_FG = 5;

    radius[m_SlicingAxis] = 0;
    radius[dimensions[0]] = DIL_FG;
    radius[dimensions[1]] = DIL_FG;

    StructuringElementType structuringElementFG;
    structuringElementFG.SetRadius(radius);
    structuringElementFG.CreateStructuringElement();

    typename BinaryDilateImageFilterType::Pointer dilateFG = BinaryDilateImageFilterType::New();
    dilateFG->SetInput(input);
    dilateFG->SetDilateValue(1);
    dilateFG->SetKernel(structuringElementFG);
    dilateFG->Update();

    typename AddImageFilterType::Pointer AddImages = AddImageFilterType::New();
    AddImages->SetInput1(dilateFG->GetOutput());
    AddImages->SetInput2(input);
    AddImages->Update();

    this->GetOutput()->Graft(AddImages->GetOutput());
}

}// end namespace



#endif
