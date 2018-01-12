#include "ImageIOWizardModel.h"
#include "GuidedNativeImageIO.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include "ImageCoordinateGeometry.h"
#include <itksys/SystemTools.hxx>

#include "ColorMap.h"
#include "ImageIODelegates.h"
#include "HistoryManager.h"
#include "GenericImageData.h"

#include "IRISException.h"
#include <sstream>


ImageIOWizardModel::ImageIOWizardModel()
{
  m_Parent = NULL;
  m_GuidedIO = NULL;
  m_LoadDelegate = NULL;
  m_SaveDelegate = NULL;
  m_Overlay = false;
  m_LoadedImage = NULL;

  // Suggested format is empty
  m_SuggestedFormat = GuidedNativeImageIO::FORMAT_COUNT;

  // Initialize various property models
  m_StickyOverlayModel = wrapGetterSetterPairAsProperty(
                           this,
                           &Self::GetStickyOverlayValue,
                           &Self::SetStickyOverlayValue);

  m_StickyOverlayColorMapModel = wrapGetterSetterPairAsProperty(
                                   this,
                                   &Self::GetStickyOverlayColorMapValue,
                                   &Self::SetStickyOverlayColorMapValue);
}


void
ImageIOWizardModel
::InitializeForSave(GlobalUIModel *parent,
                    AbstractSaveImageDelegate *delegate,
                    const char *dispName)
{
  m_Parent = parent;
  m_Mode = SAVE;
  m_HistoryName = delegate->GetHistoryName();
  m_DisplayName = dispName;
  m_GuidedIO = GuidedNativeImageIO::New();
  m_LoadDelegate = NULL;
  m_SaveDelegate = delegate;
  m_SuggestedFilename = delegate->GetCurrentFilename();
  m_Overlay = false;
  m_LoadedImage = NULL;

  m_SaveDelegate->ValidateBeforeSaving(m_SuggestedFilename, m_GuidedIO, m_Warnings);
}

void
ImageIOWizardModel
::InitializeForLoad(GlobalUIModel *parent,
                    AbstractLoadImageDelegate *delegate)
{
  m_Parent = parent;
  m_Mode = LOAD;
  m_HistoryName = delegate->GetHistoryName();
  m_DisplayName = delegate->GetDisplayName();
  m_GuidedIO = GuidedNativeImageIO::New();
  m_LoadDelegate = delegate;
  m_SaveDelegate = NULL;
  m_Overlay = delegate->IsOverlay();
  m_LoadedImage = NULL;
}

ImageIOWizardModel::~ImageIOWizardModel()
{
}

std::string
ImageIOWizardModel
::GetFilter(const char *lineEntry,
            const char *extEntry,
            const char *extSeparator,
            const char *rowSeparator)
{
  std::ostringstream ossMain;
  char buffer[1024];

  // Go through all supported formats
  for(unsigned int i=0;i < GuidedNativeImageIO::FORMAT_COUNT;i++)
    {
    FileFormat fmt = static_cast<FileFormat>(i);
    GuidedNativeImageIO::FileFormatDescriptor fd =
      GuidedNativeImageIO::GetFileFormatDescriptor(fmt);

    // Check if the file format is supported
    if(this->CanHandleFileFormat(fmt))
      {
      std::ostringstream ossLine;

      // Scan all of the separators
      size_t pos = 0;
      while(pos < fd.pattern.size())
        {
        if(pos)
          ossLine << extSeparator;
        size_t pend = fd.pattern.find(',', pos);
        std::string ext = fd.pattern.substr(pos, pend-pos);
        sprintf(buffer, extEntry, ext.c_str());
        ossLine << buffer;
        pos = (pend == std::string::npos) ? pend : pend+1;
        }

      // Append a row to the format list
      sprintf(buffer, lineEntry, fd.name.c_str(), ossLine.str().c_str());
      ossMain << buffer;
      ossMain << rowSeparator;
      }
    }

  return ossMain.str();
}

ImageIOWizardModel::FileFormat
ImageIOWizardModel::GuessFileFormat(
    const std::string &fname, bool &fileExists)
{
  // For files that don't exist, format can not be reported
  if(m_Mode == LOAD)
    {
    fileExists = itksys::SystemTools::FileExists(fname.c_str(), true);
    if(!fileExists)
      return GuidedNativeImageIO::FORMAT_COUNT;
    }

  // Look if there is prior knowledge of this image. This overrides
  // everything else
  Registry reg;
  m_Parent->GetDriver()->GetSystemInterface()->
      FindRegistryAssociatedWithFile(fname.c_str(), reg);

  // If the registry contains a file format, override with that
  FileFormat fmt =
    m_GuidedIO->GetFileFormat(reg, GuidedNativeImageIO::FORMAT_COUNT);

  // Try to select a file format accoring to the file name
  if(fmt != GuidedNativeImageIO::FORMAT_COUNT)
    return fmt;

  // If there is no prior knowledge determine the format using magic
  // numbers and extension information
  return GuidedNativeImageIO::GuessFormatForFileName(fname, m_Mode==LOAD);
}

ImageIOWizardModel::FileFormat
ImageIOWizardModel::GetFileFormatByName(const std::string &formatName) const
{
  for(int i = 0; i < GuidedNativeImageIO::FORMAT_COUNT; i++)
    {
    FileFormat fmt = (FileFormat) i;
    if(GuidedNativeImageIO::GetFileFormatDescriptor(fmt).name == formatName)
      return fmt;
    }

  return GuidedNativeImageIO::FORMAT_COUNT;
}

std::string ImageIOWizardModel::GetFileFormatName(ImageIOWizardModel::FileFormat fmt) const
{
  return GuidedNativeImageIO::GetFileFormatDescriptor(fmt).name;
}

bool ImageIOWizardModel
::CanHandleFileFormat(ImageIOWizardModel::FileFormat fmt)
{
  GuidedNativeImageIO::FileFormatDescriptor fd =
    GuidedNativeImageIO::GetFileFormatDescriptor(fmt);
  return (m_Mode == LOAD) || (m_Mode == SAVE && fd.can_write);
}

std::string
ImageIOWizardModel::GetBrowseDirectory(const std::string &file)
{
  // If empty return empty
  if(file.length() == 0)
    return file;

  // If file is a directory, return it
  std::string fn_expand = file;
  itksys::SystemTools::ConvertToUnixSlashes(fn_expand);
  if(itksys::SystemTools::FileIsDirectory(fn_expand.c_str()))
    return fn_expand;

  // Get the base name of the file
  std::string path = itksys::SystemTools::GetFilenamePath(fn_expand);
  if(itksys::SystemTools::FileIsDirectory(path.c_str()))
    return path;

  // By default, return empty string
  return std::string("");
}

std::string ImageIOWizardModel::GetDisplayName() const
{
  return m_DisplayName;
}

template<class T>
std::string triple2str(const T &triple)
{
  std::ostringstream oss;
  oss << triple[0];
  oss << " x ";
  oss << triple[1];
  oss << " x ";
  oss << triple[2];
  return oss.str();
}

std::string
ImageIOWizardModel::GetSummaryItem(ImageIOWizardModel::SummaryItem item)
{
  std::ostringstream sout;
  vnl_matrix<double> dir;
  std::string rai;

  switch(item)
    {
  case ImageIOWizardModel::SI_FILENAME:
    return m_GuidedIO->GetFileNameOfNativeImage();

  case ImageIOWizardModel::SI_DIMS:
    return triple2str(m_GuidedIO->GetNativeImage()->GetBufferedRegion().GetSize());

  case ImageIOWizardModel::SI_SPACING:
    return triple2str(m_GuidedIO->GetNativeImage()->GetSpacing());

  case ImageIOWizardModel::SI_ORIGIN:
    return triple2str(m_GuidedIO->GetNativeImage()->GetOrigin());

  case ImageIOWizardModel::SI_ORIENT:
    dir = m_GuidedIO->GetNativeImage()->GetDirection().GetVnlMatrix();
    rai = ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(dir);
    if(ImageCoordinateGeometry::IsDirectionMatrixOblique(dir))
      sout << "Oblique (closest to " << rai << ")";
    else
      sout << rai;
    return sout.str();

  case ImageIOWizardModel::SI_ENDIAN:
    return (m_GuidedIO->GetByteOrderInNativeImage()
            == itk::ImageIOBase::BigEndian)
        ? "Big Endian" : "Little Endian";

  case ImageIOWizardModel::SI_DATATYPE:
    if(m_GuidedIO->GetComponentTypeInNativeImage()
       != itk::ImageIOBase::UNKNOWNCOMPONENTTYPE)
      {
      // There actually is a type in the IO object
      return m_GuidedIO->GetComponentTypeAsStringInNativeImage();
      }
    else
      {
      // TODO: This is a workaround on an itk bug with RawImageIO
      // TODO: fix this (get the text selected for the raw image)
      return "Unknown";
      }

  case ImageIOWizardModel::SI_COMPONENTS:
    sout << m_GuidedIO->GetNumberOfComponentsInNativeImage();
    return sout.str();

  case ImageIOWizardModel::SI_FILESIZE:
    sout << (m_GuidedIO->GetFileSizeOfNativeImage() / 1024.0) << " Kb";
    return sout.str();
    }

  return std::string("");
}

void ImageIOWizardModel::SetSelectedFormat(ImageIOWizardModel::FileFormat fmt)
{
  GuidedNativeImageIO::SetFileFormat(m_Registry, fmt);
}


ImageIOWizardModel::FileFormat ImageIOWizardModel::GetSelectedFormat()
{
  return GuidedNativeImageIO::GetFileFormat(m_Registry);
}



void ImageIOWizardModel::LoadImage(std::string filename)
{
  // There is no loaded image to start with
  m_LoadedImage = NULL;

  try
  {
    // Clear the warnings
    m_Warnings.clear();

    // Load the header
    m_GuidedIO->ReadNativeImageHeader(filename.c_str(), m_Registry);

    // Check if the header is valid
    m_LoadDelegate->ValidateHeader(m_GuidedIO, m_Warnings);

    // Remove current data
    m_LoadDelegate->UnloadCurrentImage();

    // Load the data from the image
    m_GuidedIO->ReadNativeImageData();

    // Validate the image data
    m_LoadDelegate->ValidateImage(m_GuidedIO, m_Warnings);

    // Update the application
    m_LoadedImage =
        m_LoadDelegate->UpdateApplicationWithImage(m_GuidedIO);

    // Save the IO hints to the registry
    Registry regAssoc;
    SystemInterface *si = m_Parent->GetDriver()->GetSystemInterface();
    si->FindRegistryAssociatedWithFile(
          m_GuidedIO->GetFileNameOfNativeImage().c_str(), regAssoc);
    regAssoc.Folder("Files.Grey").Update(m_Registry);
    si->AssociateRegistryWithFile(
          m_GuidedIO->GetFileNameOfNativeImage().c_str(), regAssoc);

    // Also place the IO hints into the layer
    m_LoadedImage->SetIOHints(m_Registry);
  }
  catch(IRISException &excIRIS)
  {
    throw excIRIS;
  }
  catch(std::exception &exc)
  {
    throw IRISException("Error: exception occured during image IO. "
                        "Exception: %s", exc.what());
  }
}

void ImageIOWizardModel::SaveImage(std::string filename)
{
  try
  {
  m_SaveDelegate->SaveImage(filename, m_GuidedIO, m_Registry, m_Warnings);
  }
  catch(std::exception &exc)
  {
    throw IRISException("Error: exception occured during image IO. "
                        "Exception: %s", exc.what());
  }
}

bool ImageIOWizardModel::CheckImageValidity()
{
  IRISWarningList warn;
  m_LoadDelegate->ValidateHeader(m_GuidedIO, warn);

  return true;
}

void ImageIOWizardModel::Reset()
{
  m_Registry.Clear();
}

void ImageIOWizardModel::ProcessDicomDirectory(const std::string &filename,
                                               itk::Command *progressCommand)
{
  // Get the directory
  std::string dir = GetBrowseDirectory(filename);

  // Get the registry
  try
  {
    m_GuidedIO->ParseDicomDirectory(dir, progressCommand);
  }
  catch (IRISException &ei)
  {
    throw ei;
  }
  catch (std::exception &e)
  {
    throw IRISException("Error: exception occured when parsing DICOM directory. "
                        "Exception: %s", e.what());
  }
}

std::list<std::string>
ImageIOWizardModel
::GetFoundDicomSeriesIds()
{
  // Get the DICOM registry from the GuidedIO
  typedef GuidedNativeImageIO::DicomDirectoryParseResult ParseResult;
  const ParseResult &pr = m_GuidedIO->GetLastDicomParseResult();

  std::list<std::string> result;
  for(ParseResult::SeriesMapType::const_iterator it = pr.SeriesMap.begin();
      it != pr.SeriesMap.end(); ++it)
    result.push_back(it->first);

  return result;
}

Registry
ImageIOWizardModel
::GetFoundDicomSeriesMetaData(const std::string &series_id)
{
  // Get the DICOM registry from the GuidedIO
  typedef GuidedNativeImageIO::DicomDirectoryParseResult ParseResult;
  const ParseResult &pr = m_GuidedIO->GetLastDicomParseResult();

  // Registry result
  Registry r;

  // Find the metadata
  ParseResult::SeriesMapType::const_iterator it = pr.SeriesMap.find(series_id);
  if(it != pr.SeriesMap.end())
    r.Update(it->second.MetaData);

  return r;
}

void ImageIOWizardModel
::LoadDicomSeries(const std::string &filename,
                  const std::string &series_id)
{
  // Get the DICOM registry from the GuidedIO
  typedef GuidedNativeImageIO::DicomDirectoryParseResult ParseResult;
  const ParseResult &pr = m_GuidedIO->GetLastDicomParseResult();

  // Get the metadata for the current series
  ParseResult::SeriesMapType::const_iterator itc = pr.SeriesMap.find(series_id);
  if(itc == pr.SeriesMap.end())
    throw IRISException("DICOM series id %s not found, logic error",
                        series_id.c_str());
  Registry meta_current = itc->second.MetaData;

  // Set up the registry for DICOM IO
  m_Registry["DICOM.SeriesId"] << meta_current["SeriesId"][""];
  m_Registry.Folder("DICOM.SeriesFiles").PutArray(
        meta_current.Folder("SeriesFiles").GetArray(std::string()));

  // Store information about the entire dicom diretory into a separate subfolder.
  // This is to allow subsequent quick loading of other DICOM series in the same
  // directory, e.g., through a menu item on the main menu
  m_Registry["DICOM.DirectoryInfo.ArraySize"] << pr.SeriesMap.size();
  int i = 0;
  for(ParseResult::SeriesMapType::const_iterator it = pr.SeriesMap.begin();
      it != pr.SeriesMap.end(); ++it, ++i)
    {
    Registry &r = m_Registry.Folder(m_Registry.Key("DICOM.DirectoryInfo.Entry[%d]", i));
    r.Update(it->second.MetaData);
    }

  // Set the format to DICOM
  SetSelectedFormat(GuidedNativeImageIO::FORMAT_DICOM_DIR);

  // Get the directory
  std::string dir = GetBrowseDirectory(filename);

  // Call the main load method
  this->LoadImage(dir);

  // DICOM filenames are meaningless. Assign a nickname based on series name
  if(m_LoadedImage->GetCustomNickname().length() == 0)
    {
    m_LoadedImage->SetCustomNickname(meta_current["SeriesDescription"][""]);
    }

}

unsigned long ImageIOWizardModel::GetFileSizeInBytes(const std::string &file)
{
  return itksys::SystemTools::FileLength(file.c_str());
}

bool ImageIOWizardModel::IsImageLoaded() const
{
  // TODO: this may have to change based on validity checks
  return m_GuidedIO->IsNativeImageLoaded();
}

void ImageIOWizardModel::Finalize()
{
}

bool ImageIOWizardModel::GetStickyOverlayValue(bool &value)
{
  // Make sure the image has already been loaded
  if(!m_LoadedImage)
    return false;

  // Return the stickiness value
  value = m_LoadedImage->IsSticky();
  return true;
}

void ImageIOWizardModel::SetStickyOverlayValue(bool value)
{
  assert(m_LoadedImage);
  m_LoadedImage->SetSticky(value);
}

bool ImageIOWizardModel::GetStickyOverlayColorMapValue(std::string &value)
{
  // Make sure the image has already been loaded
  if(!m_LoadedImage || !m_LoadedImage->IsSticky())
    return false;

  // Get the display mapping policy (to get a color map)
  ColorMap *cmap = m_LoadedImage->GetDefaultScalarRepresentation()->GetColorMap();
  if(!cmap)
    return false;

  // Return the color map preset
  value = ColorMap::GetPresetName(cmap->GetSystemPreset());
  return true;
}

void ImageIOWizardModel::SetStickyOverlayColorMapValue(std::string value)
{
  assert(m_LoadedImage && m_LoadedImage->IsSticky());

  ScalarImageWrapperBase *base = m_LoadedImage->GetDefaultScalarRepresentation();
  for(int i = 0; i < ColorMap::COLORMAP_CUSTOM; i++)
    if(value == ColorMap::GetPresetName((ColorMap::SystemPreset) i))
      {
      base->GetColorMap()->SetToSystemPreset((ColorMap::SystemPreset) i);
      return;
      }
}
