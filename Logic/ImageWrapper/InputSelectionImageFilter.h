#ifndef INPUTSELECTIONIMAGEFILTER_H
#define INPUTSELECTIONIMAGEFILTER_H

#include "itkImageToImageFilter.h"
#include "SNAPCommon.h"
#include <map>

/**
 * This filter selects one of its inputs and presents it as an output. It is
 * used when there are several alternative pipelines that can be used to
 * produce an output. Updating the output will Update() the correct pipeline.
 *
 * The class is templated over the type TTag, which is an arbitrary type
 * that is used to identify input pipelines. This allows pipelines to be associated
 * with non-integer values (structs, enums, etc.)
 */
template <class TInputImage, typename TTag = int>
class InputSelectionImageFilter
    : public itk::ImageToImageFilter<TInputImage,TInputImage>
{
public:

  typedef InputSelectionImageFilter<TInputImage, TTag> Self;
  typedef itk::ImageToImageFilter<TInputImage,TInputImage> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  typedef TInputImage InputImageType;
  typedef TInputImage OutputImageType;

  itkNewMacro(Self)
  itkStaticConstMacro(ImageDimension, unsigned int, InputImageType::ImageDimension);

  typedef TTag TagType;

  /**
   * Add a selectable input identified with a tag
   */
  void AddSelectableInput(TagType tag, InputImageType *input);

  /**
   * Remove all selectable inputs
   */
  void RemoveAllSelectableInputs();

  /**
   * Set the selected input - must be one of the inputs added with AddInput!
   * Defaults to the first input added.
   */
  void SetSelectedInput(TagType &tag);

  itkGetMacro(SelectedInput, TagType)

  /**
   * Generate the data - by selecting an input and presenting it as output
   */
  void GenerateData();

protected:

  InputSelectionImageFilter();
  virtual ~InputSelectionImageFilter() {};

  TagType m_SelectedInput;

  // A mapping from tag types to inputs
  typedef std::map<TagType, SmartPtr<InputImageType> > TagMap;
  TagMap m_TagMap;
};

#endif // INPUTSELECTIONIMAGEFILTER_H
