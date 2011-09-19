#include "ImageIOWizardModel.h"
#include "GuidedNativeImageIO.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SystemInterface.h"
#include "ImageCoordinateGeometry.h"
#include <itksys/SystemTools.hxx>
#include <sstream>

ImageIOWizardModel::ImageIOWizardModel(
    GlobalUIModel *parent, Mode mode, const char *name)
{
  m_Parent = parent;
  m_Mode = mode;
  m_HistoryName = name;
  m_GuidedIO = new GuidedNativeImageIO();
}

ImageIOWizardModel::~ImageIOWizardModel()
{
  delete m_GuidedIO;
}

std::string
ImageIOWizardModel
::GetFilter(const char *lineEntry,
            const char *extEntry,
            const char *extSeparator,
            const char *rowSeparator)
{
  std::ostringstream ossMain, ossAllImageFiles;
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

      // Append the extension to 'All image files'
      if(ossAllImageFiles.tellp())
        ossAllImageFiles << extSeparator;
      ossAllImageFiles << ossLine.str();

      // Append a row to the format list
      sprintf(buffer, lineEntry, fd.name.c_str(), ossLine.str().c_str());
      ossMain << buffer;
      ossMain << rowSeparator;
      }
    }

  // Add global selectors
  sprintf(buffer, lineEntry, "All 3D Image Files",
          ossAllImageFiles.str().c_str());
  ossMain << buffer;
  ossMain << rowSeparator;

  // Add global selectors
  sprintf(buffer, lineEntry, "All Files", "*");
  ossMain << buffer;

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

bool ImageIOWizardModel
::CanHandleFileFormat(ImageIOWizardModel::FileFormat fmt)
{
  GuidedNativeImageIO::FileFormatDescriptor fd =
    GuidedNativeImageIO::GetFileFormatDescriptor(fmt);
  return (m_Mode == LOAD) || (m_Mode == SAVE && fd.can_write);
}

std::string
ImageIOWizardModel::GetBrowseDirectory(std::string &file)
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

ImageIOWizardModel::HistoryType ImageIOWizardModel::GetHistory() const
{
  return m_Parent->GetDriver()->GetSystemInterface()
      ->GetHistory(m_HistoryName.c_str());
}

void ImageIOWizardModel::SetSelectedFormat(ImageIOWizardModel::FileFormat fmt)
{
  GuidedNativeImageIO::SetFileFormat(m_Registry, fmt);
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

  case ImageIOWizardModel::SI_FILESIZE:
    sout << (m_GuidedIO->GetFileSizeOfNativeImage() / 1024.0) << " Kb";
    return sout.str();
    }

  return std::string("");
}

void ImageIOWizardModel::LoadImage(std::string filename)
{
  m_GuidedIO->ReadNativeImage(filename.c_str(), m_Registry);
}

bool ImageIOWizardModel::CheckImageValidity()
{
  return true;
}

void ImageIOWizardModel::Reset()
{
  m_Registry.Clear();
}

