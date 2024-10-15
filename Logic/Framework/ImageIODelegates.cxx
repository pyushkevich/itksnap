#include "ImageIODelegates.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "HistoryManager.h"
#include "IRISImageData.h"
#include "ImageWrapperTraits.h"
#include <itkImageIOBase.h>
#include <itkImageBase.h>


/* =============================
   Abstract Classes
   ============================= */

void LoadAnatomicImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  typedef itk::ImageIOBase IOB;

  itk::IOComponentEnum ct = io->GetComponentTypeInNativeImage();
  if(ct != IOB::UCHAR &&
     ct != IOB::CHAR &&
     ct != IOB::USHORT &&
     ct != IOB::SHORT &&
     ct != IOB::FLOAT)
    {
    std::ostringstream oss;
    oss << ct;
    wl.push_back(
          IRISWarning(
            "Warning: Possible Loss of Precision."
            "The file you opened represents image data using the '%s' data type, "
            "but ITK-SNAP only supports 16-bit integer and 32-bit floating point data types. "
            "Intensity values reported in ITK-SNAP may differ from the "
            "actual values in the image.", oss.str().c_str()));
    }
}


/* =============================
   MAIN Image
   ============================= */
void
LoadMainImageDelegate
::UnloadCurrentImage()
{
  m_Driver->UnloadMainImage();
}

ImageWrapperBase *LoadMainImageDelegate::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  // Update the IRIS driver
  m_Driver->UpdateIRISMainImage(io, this->GetMetaDataRegistry());

  // Return the main image
  return m_Driver->GetIRISImageData()->GetMain();
}

LoadMainImageDelegate::LoadMainImageDelegate()
{
  this->m_HistoryName = "AnatomicImage";
  this->m_DisplayName = "Main Image";
}

void
LoadMainImageDelegate
::ConfigureImageIO(GuidedNativeImageIO *io)
{
  if (m_Load4DAsMultiComponent)
    io->SetLoad4DAsMultiComponent(true);
  else if (m_LoadMultiComponentAs4D)
    io->SetLoadMultiComponentAs4D(true);
}


/* =============================
   OVERLAY Image
   ============================= */

void
LoadOverlayImageDelegate
::UnloadCurrentImage()
{
  // TODO: what do we do here? Are we always adding an overlay? What about
  // on an undo in a wizard?
}

ImageWrapperBase *LoadOverlayImageDelegate::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  // Load the overlay
  m_Driver->AddIRISOverlayImage(io, this->GetMetaDataRegistry());

  // Return it
  return m_Driver->GetIRISImageData()->GetLastOverlay();
}


void
LoadOverlayImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  // Do the parent's check
  LoadAnatomicImageDelegate::ValidateHeader(io, wl);

  // The check below is commented out because we no longer require the overlays
  // to be in the same space as the main image
  /*
  // Now check for dimensions mismatch
  GenericImageData *id = m_Driver->GetCurrentImageData();

  // Check the dimensions, throw exception
  Vector3ui szSeg = io->GetDimensionsOfNativeImage();
  Vector3ui szMain = id->GetMain()->GetSize();
  if(szSeg != szMain)
    {
    throw IRISException("Error: Mismatched Dimensions. "
                        "The size of the overlay image (%d x %d x %d) "
                        "does not match the size of the main image "
                        "(%d x %d x %d). Images must have the same dimensions.",
                        szSeg[0], szSeg[1], szSeg[2],
        szMain[0], szMain[1], szMain[2]);
    }
  */
}

LoadOverlayImageDelegate::LoadOverlayImageDelegate()
{
  this->m_HistoryName = "AnatomicImage";
  this->m_DisplayName = "Additional Image";
}

/* =============================
   SEGMENTATION Image
   ============================= */

void
LoadSegmentationImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  GenericImageData *id = m_Driver->GetCurrentImageData();

  // Get the dimensions of the main image
  Vector3ui szMain = id->GetMain()->GetSize();
  unsigned int ntMain =  id->GetMain()->GetNumberOfTimePoints();

  // Get the dimensions of the segmentation
  Vector4ui szSeg4D = io->GetDimensionsOfNativeImage();
  Vector3ui szSeg = szSeg4D.extract(3);
  unsigned int ntSeg = szSeg4D[3];

  // The 3D dimensions must match
  if(szSeg != szMain)
    {
    throw IRISException("Error: Mismatched Dimensions. "
                        "The size of the segmentation image (%d x %d x %d) "
                        "does not match the size of the main image "
                        "(%d x %d x %d). Images must have the same dimensions.",
                        szSeg[0], szSeg[1], szSeg[2],
                        szMain[0], szMain[1], szMain[2]);
    }

  // Check the number of components
  if(io->GetNumberOfComponentsInNativeImage() != 1)
    {
    throw IRISException("Error: Multicomponent Image. "
                        "The segmentation image has multiple (%d) components, "
                        "but only one component is supported by ITK-SNAP.",
                        io->GetNumberOfComponentsInNativeImage());
    }
  
  // The number of components must also match
  if(ntMain != ntSeg)
    {
    if(ntSeg > 1)
      {
      // Nothing can be done
      throw IRISException("Error: Mismatched number of time points. "
                          "The number of time points (%d) in the segmentation image "
                          "does not match the number of time points (%d) in the main image. "
                          "Images must have the same number of time points.",
                          ntSeg, ntMain);
      }
    else
      {
      // Just issue a warning and proceed
      wl.push_back(IRISWarning(
                     "Warning: Mismatched number of time points."
                     "The main image has %d time points but the segmentation image has only one time point. "
                     "The current time point in the segmentation has been replaced by the image you are loading. ", ntMain));
      }
    }

}

void
LoadSegmentationImageDelegate
::ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  // Get the two images to compare
  GenericImageData *id = m_Driver->GetCurrentImageData();
  itk::ImageBase<4> *main = id->GetMain()->GetImage4DBase();
  itk::ImageBase<4> *native = io->GetNativeImage();

  // Check the header properties
  // Check if there is a discrepancy in the header fields. This will not
  // preclude the user from loading the image, but it will generate a
  // warning, hopefully leading users to adopt more flexible file formats
  bool match_spacing = true, match_origin = true, match_direction = true;

  for(unsigned int i = 0; i < 3; i++)
    {
    if(main->GetSpacing()[i] != native->GetSpacing()[i])
      match_spacing = false;

    if(main->GetOrigin()[i] != native->GetOrigin()[i])
      match_origin = false;

    for(size_t j = 0; j < 3; j++)
      {
      double diff = fabs(main->GetDirection()(i,j) - native->GetDirection()(i,j));
      if(diff > 1.0e-4)
        match_direction = false;
      }
    }

  if(!match_spacing || !match_origin || !match_direction)
    {
    // Come up with a warning message
    std::string object, verb;
    if(!match_spacing && !match_origin && !match_direction)
      { object = "spacing, origin and orientation"; }
    else if (!match_spacing && !match_origin)
      { object = "spacing and origin"; }
    else if (!match_spacing && !match_direction)
      { object = "spacing and orientation"; }
    else if (!match_origin && !match_direction)
      { object = "origin and orientation";}
    else if (!match_spacing)
      { object = "spacing"; }
    else if (!match_direction)
      { object = "orientation";}
    else if (!match_origin)
      { object = "origin"; }

    // Create an alert box
    wl.push_back(IRISWarning(
                   "Warning: Header Mismatch."
                   "There is a mismatch between the header of the image that you are "
                   "loading and the header of the main image currently open in ITK-SNAP. "
                   "The images have different %s. "
                   "ITK-SNAP will ignore the header in the image you are loading.",
                   object.c_str()));
    }
}

ImageWrapperBase *LoadSegmentationImageDelegate::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  if(m_Driver->IsSnakeModeActive())
    return m_Driver->UpdateSNAPSegmentationImage(io);
  else
    return m_Driver->UpdateIRISSegmentationImage(io, this->GetMetaDataRegistry(), m_AdditiveMode);
}

LoadSegmentationImageDelegate::LoadSegmentationImageDelegate()
{
  this->m_HistoryName = "LabelImage";
  this->m_DisplayName = "Segmentation Image";
  this->m_AdditiveMode = false;
}

void
LoadSegmentationImageDelegate
::UnloadCurrentImage()
{
  // It is unnecessary to unload the current segmentation image, because that
  // just will reinitialize it with zeros. It's safe to just do nothing.
}

bool
LoadSegmentationImageDelegate
::CanLoadOverwriteUnsavedChanges(GuidedNativeImageIO *io, std::string filename)
{
  auto header = io->PeekHeader(filename);
  auto nDimIncoming = header->GetNumberOfDimensions();
  auto nt = m_Driver->GetNumberOfTimePoints();

  // for 4d workspace
  if (nt > 1)
    {
    auto layer = m_Driver->GetSelectedSegmentationLayer();
    auto crntTP = m_Driver->GetCursorTimePoint();
    // true case 1: incoming image is 4d
    // true case 2: incoming 3d but unsaved changes are in the current time point
    if (nDimIncoming > 3 || layer->HasUnsavedChanges(crntTP))
      return true;
    }

  return false;
}


void
DefaultSaveImageDelegate::Initialize(
    IRISApplication *driver,
    ImageWrapperBase *wrapper,
    const std::string &histname,
    bool trackInLocalHistory)
{
  AbstractSaveImageDelegate::Initialize(driver);
  m_Wrapper = wrapper;
  m_Track = trackInLocalHistory;
  this->AddHistoryName(histname);
}

void DefaultSaveImageDelegate::AddHistoryName(const std::string &histname)
{
  m_HistoryNames.push_back(histname);
}

const char *DefaultSaveImageDelegate::GetHistoryName()
{
  return m_HistoryNames.front().c_str();
}

void DefaultSaveImageDelegate
::ValidateBeforeSaving(
    const std::string &fname, GuidedNativeImageIO *io, IRISWarningList &wl)
{
}


void DefaultSaveImageDelegate
::SaveImage(const std::string &fname, GuidedNativeImageIO *io,
            Registry &reg, IRISWarningList &wl)
{
  try
    {
    m_SaveSuccessful = false;
    m_Wrapper->WriteToFile(fname.c_str(), reg);
    m_SaveSuccessful = true;

    m_Wrapper->SetFileName(fname);
    for(std::list<std::string>::const_iterator it = m_HistoryNames.begin();
        it != m_HistoryNames.end(); ++it)
      {
      m_Driver->GetHistoryManager()->UpdateHistory(*it, fname, m_Track);
      }
    }
  catch(std::exception &exc)
    {
    throw exc;
   }
}

const char *DefaultSaveImageDelegate::GetCurrentFilename()
{
  return m_Wrapper->GetFileName();
}


/* =============================
   AbstractReloadImageDelegate
   ============================= */
void
AbstractReloadWrapperDelegate
::ValidateHeader(IRISWarningList &wl)
{
  Registry dummyReg;
  m_IO->ReadNativeImageHeader(m_Filename.c_str(), dummyReg, nullptr);
  auto headerFile = m_IO->GetIOBase();

  auto imageNative = m_Wrapper->GetImage4DBase();

  // compare dimension, spacing, origin and direction
  auto regNative = imageNative->GetLargestPossibleRegion();
  auto dimNative = regNative.GetSize();

  // Check the header properties
  // Check if there is a discrepancy in the header fields.
  bool match_spacing = true, match_origin = true, match_direction = true, match_dimension = true;

  auto ndimFile = headerFile->GetNumberOfDimensions();
  for(unsigned int i = 0; i < (ndimFile < 4 ? ndimFile : 4); i++)
    {
    match_spacing = (headerFile->GetSpacing(i) == imageNative->GetSpacing()[i]);
    match_origin = (headerFile->GetOrigin(i) == imageNative->GetOrigin()[i]);
    match_dimension = (headerFile->GetDimensions(i) == dimNative[i]);

    for(size_t j = 0; j < (ndimFile < 4 ? ndimFile : 4); j++)
      {
      double diff = fabs(headerFile->GetDirection(i)[j] - imageNative->GetDirection()(i,j));
      match_direction = (diff <= 1.0e-4);
      }
    }

  if(!match_spacing || !match_origin || !match_direction || ! match_dimension)
    {
    std::ostringstream oss;
    oss << "Following header elements mismatch between header of the file and the "
        << "image header currently open in ITK-SNAP: ("
        << (match_spacing ? "" : "spacing ")
        << (match_origin ? "" : "origin ")
        << (match_direction ? "" : "direction ")
        << (match_dimension ? "" : "dimension")
        << "). Image cannot be reloaded from the file!";
    // Create an alert box
    throw IRISException("Validation failed during image file reload: %s", oss.str().c_str());
    }
}


/* =============================
   RELOAD anatomic wrapper
   ============================= */


void
ReloadAnatomicWrapperDelegate
::UpdateWrapper()
{
  m_IO->ReadNativeImageData();

  // this logic tracks GenericImageData::CreateAnatomicWrapper
  switch(m_IO->GetComponentTypeInNativeImage())
    {
    case itk::IOComponentEnum::UCHAR:  UpdateWrapperInternal<unsigned char>(); break;
    case itk::IOComponentEnum::CHAR:   UpdateWrapperInternal<char>(); break;
    case itk::IOComponentEnum::USHORT: UpdateWrapperInternal<unsigned short>(); break;
    case itk::IOComponentEnum::SHORT:  UpdateWrapperInternal<short>(); break;
    default: UpdateWrapperInternal<float>(); break;
    }
}

template<typename TPixel>
void
ReloadAnatomicWrapperDelegate
::UpdateWrapperInternal()
{
  if (m_IO->GetNumberOfComponentsInNativeImage() > 1)
    UpdateWrapperWithTraits<AnatomicImageWrapperTraits<TPixel>>();
  else
    UpdateWrapperWithTraits<AnatomicScalarImageWrapperTraits<TPixel>>();
}

template<typename TTraits>
void
ReloadAnatomicWrapperDelegate
::UpdateWrapperWithTraits()
{
  using WrapperType = typename TTraits::WrapperType;
  using Image4DType = typename WrapperType::Image4DType;

  RescaleNativeImageToIntegralType<Image4DType> rescaler;
  typename Image4DType::Pointer image4d = rescaler(m_IO);

  auto *aw = dynamic_cast<WrapperType*>(m_Wrapper.GetPointer());

  if (!aw)
    {
    std::ostringstream oss;
    oss << "Cannot cast wrapper to: \""
        << typeid(WrapperType).name() << "\"";
    throw IRISException("Error reloading image from file: %s", oss.str().c_str());
    }

  // Maintain the reference space and transform when reloading an image
  auto *old_tform = aw->GetITKTransform();
  auto *old_refspace = aw->GetReferenceSpace();
  if(old_refspace == aw->GetImageBase())
    old_refspace = nullptr;
  aw->SetImage4D(image4d,
                 old_refspace,
                 const_cast<typename WrapperType::ITKTransformType *>(old_tform));

  // This line takes care of stale pointers to reference_space in image wrappers
  if(m_Wrapper == m_Driver->GetCurrentImageData()->GetMain())
    m_Driver->GetCurrentImageData()->UpdateReferenceImageInAllLayers();

  m_Driver->SetCursorPosition(m_Driver->GetCursorPosition(), true);
  m_Driver->InvokeEvent(LayerChangeEvent()); // important, to trigger renderer rebuild assemblies
}



/* =============================
   RELOAD segmentation wrapper
   ============================= */

void
ReloadSegmentationWrapperDelegate
::UpdateWrapper()
{
  m_IO->ReadNativeImageData();

  auto labelWrapper = dynamic_cast<LabelImageWrapper*>(m_Wrapper.GetPointer());
  if (!labelWrapper)
    {
    throw IRISException("Error reloading segmentation from file: Error casting to LabelIamgeWrapper!");
    }


  auto labelImage = m_Driver->GetIRISImageData()->CompressSegmentation(m_IO);
  labelWrapper->SetImage4D(labelImage);
  m_Driver->SetCursorPosition(m_Driver->GetCursorPosition(), true);
  m_Driver->InvokeEvent(LayerChangeEvent()); // important, to trigger renderer rebuild assemblies
}
