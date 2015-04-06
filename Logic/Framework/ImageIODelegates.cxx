#include "ImageIODelegates.h"
#include "GuidedNativeImageIO.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "HistoryManager.h"


/* =============================
   Abstract Classes
   ============================= */

void LoadAnatomicImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  typedef itk::ImageIOBase IOB;

  IOB::IOComponentType ct = io->GetComponentTypeInNativeImage();
  if(ct > IOB::SHORT)
    {
    wl.push_back(
          IRISWarning(
            "Warning: Loss of Precision."
            "You are opening an image with 32-bit or greater precision, "
            "but ITK-SNAP only provides 16-bit precision. "
            "Intensity values reported in ITK-SNAP may differ slightly from the "
            "actual values in the image."
            ));
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

void
LoadMainImageDelegate
::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  // Update the IRIS driver
  m_Driver->UpdateIRISMainImage(io, this->GetMetaDataRegistry());
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

void
LoadOverlayImageDelegate
::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  m_Driver->AddIRISOverlayImage(io, this->GetMetaDataRegistry());
}


void
LoadOverlayImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  // Do the parent's check
  LoadAnatomicImageDelegate::ValidateHeader(io, wl);

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
}



/* =============================
   Co-Registered Overlay Image
   ============================= */

void
LoadCoregisteredOverlayImageDelegate
::UnloadCurrentImage()
{
}

void
LoadCoregisteredOverlayImageDelegate
::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  m_Driver->AddIRISCoregOverlayImage(io, this->GetMetaDataRegistry());
}


void
LoadCoregisteredOverlayImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  // Do the parent's check
  LoadAnatomicImageDelegate::ValidateHeader(io, wl);
}

/* =============================
   SEGMENTATION Image
   ============================= */

void
LoadSegmentationImageDelegate
::ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  GenericImageData *id = m_Driver->GetCurrentImageData();

  // Check the dimensions, throw exception
  Vector3ui szSeg = io->GetDimensionsOfNativeImage();
  Vector3ui szMain = id->GetMain()->GetSize();
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
}

void
LoadSegmentationImageDelegate
::ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl)
{
  // Get the two images to compare
  GenericImageData *id = m_Driver->GetCurrentImageData();
  itk::ImageBase<3> *main = id->GetMain()->GetImageBase();
  itk::ImageBase<3> *native = io->GetNativeImage();

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

void
LoadSegmentationImageDelegate
::UpdateApplicationWithImage(GuidedNativeImageIO *io)
{
  m_Driver->UpdateIRISSegmentationImage(io);
}

void
LoadSegmentationImageDelegate
::UnloadCurrentImage()
{
  // It is unnecessary to unload the current segmentation image, because that
  // just will reinitialize it with zeros. It's safe to just do nothing.
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


