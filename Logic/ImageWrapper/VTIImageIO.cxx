#include "VTIImageIO.h"

#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkType.h>

#include <itksys/SystemTools.hxx>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers: map between VTK scalar type and ITK IO component type
// ---------------------------------------------------------------------------

static itk::IOComponentEnum VTKTypeToITK(int vtkType)
{
  switch(vtkType)
    {
    case VTK_CHAR:           return itk::IOComponentEnum::CHAR;
    case VTK_SIGNED_CHAR:    return itk::IOComponentEnum::CHAR;
    case VTK_UNSIGNED_CHAR:  return itk::IOComponentEnum::UCHAR;
    case VTK_SHORT:          return itk::IOComponentEnum::SHORT;
    case VTK_UNSIGNED_SHORT: return itk::IOComponentEnum::USHORT;
    case VTK_INT:            return itk::IOComponentEnum::INT;
    case VTK_UNSIGNED_INT:   return itk::IOComponentEnum::UINT;
    case VTK_LONG:           return itk::IOComponentEnum::LONG;
    case VTK_UNSIGNED_LONG:  return itk::IOComponentEnum::ULONG;
    case VTK_FLOAT:          return itk::IOComponentEnum::FLOAT;
    case VTK_DOUBLE:         return itk::IOComponentEnum::DOUBLE;
    default:                 return itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE;
    }
}

static int ITKTypeToVTK(itk::IOComponentEnum itkType)
{
  switch(itkType)
    {
    case itk::IOComponentEnum::CHAR:   return VTK_CHAR;
    case itk::IOComponentEnum::UCHAR:  return VTK_UNSIGNED_CHAR;
    case itk::IOComponentEnum::SHORT:  return VTK_SHORT;
    case itk::IOComponentEnum::USHORT: return VTK_UNSIGNED_SHORT;
    case itk::IOComponentEnum::INT:    return VTK_INT;
    case itk::IOComponentEnum::UINT:   return VTK_UNSIGNED_INT;
    case itk::IOComponentEnum::LONG:   return VTK_LONG;
    case itk::IOComponentEnum::ULONG:  return VTK_UNSIGNED_LONG;
    case itk::IOComponentEnum::FLOAT:  return VTK_FLOAT;
    case itk::IOComponentEnum::DOUBLE: return VTK_DOUBLE;
    default:                           return VTK_FLOAT;
    }
}

// ---------------------------------------------------------------------------

bool VTIImageIO::CanReadFile(const char *filename)
{
  vtkNew<vtkXMLImageDataReader> reader;
  return reader->CanReadFile(filename) != 0;
}

bool VTIImageIO::CanWriteFile(const char *filename)
{
  std::string ext = itksys::SystemTools::GetFilenameLastExtension(filename);
  return ext == ".vti";
}

void VTIImageIO::ReadImageInformation()
{
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(m_FileName.c_str());
  reader->Update();

  m_CachedImage = reader->GetOutput();

  int dims[3];
  m_CachedImage->GetDimensions(dims);

  double spacing[3];
  m_CachedImage->GetSpacing(spacing);

  double origin[3];
  m_CachedImage->GetOrigin(origin);

  int ncomp   = m_CachedImage->GetNumberOfScalarComponents();
  int vtkType = m_CachedImage->GetScalarType();

  this->SetNumberOfDimensions(3);
  for(int i = 0; i < 3; i++)
    {
    this->SetDimensions(i, static_cast<unsigned int>(dims[i]));
    this->SetSpacing(i, spacing[i]);
    this->SetOrigin(i, origin[i]);
    std::vector<double> dir(3, 0.0);
    dir[i] = 1.0;
    this->SetDirection(i, dir);
    }

  this->SetComponentType(VTKTypeToITK(vtkType));

  if(ncomp == 1)
    {
    this->SetPixelType(itk::IOPixelEnum::SCALAR);
    this->SetNumberOfComponents(1);
    }
  else
    {
    this->SetPixelType(itk::IOPixelEnum::VECTOR);
    this->SetNumberOfComponents(ncomp);
    }
}

void VTIImageIO::Read(void *buffer)
{
  // Re-read if not cached (e.g. when used standalone without ReadImageInformation)
  if(!m_CachedImage)
    {
    vtkNew<vtkXMLImageDataReader> reader;
    reader->SetFileName(m_FileName.c_str());
    reader->Update();
    m_CachedImage = reader->GetOutput();
    }

  vtkIdType nbytes = m_CachedImage->GetNumberOfPoints()
    * m_CachedImage->GetNumberOfScalarComponents()
    * m_CachedImage->GetScalarSize();

  std::memcpy(buffer, m_CachedImage->GetScalarPointer(), static_cast<size_t>(nbytes));

  // Release the cached copy
  m_CachedImage = nullptr;
}

void VTIImageIO::WriteImageInformation()
{
  // Nothing needed; all handled in Write()
}

void VTIImageIO::Write(const void *buffer)
{
  vtkNew<vtkImageData> img;
  img->SetDimensions(
    static_cast<int>(this->GetDimensions(0)),
    static_cast<int>(this->GetDimensions(1)),
    static_cast<int>(this->GetDimensions(2)));
  img->SetSpacing(this->GetSpacing(0), this->GetSpacing(1), this->GetSpacing(2));
  img->SetOrigin(this->GetOrigin(0),  this->GetOrigin(1),  this->GetOrigin(2));

  img->AllocateScalars(ITKTypeToVTK(this->GetComponentType()),
                       static_cast<int>(this->GetNumberOfComponents()));

  vtkIdType nbytes = img->GetNumberOfPoints()
    * static_cast<vtkIdType>(this->GetNumberOfComponents())
    * static_cast<vtkIdType>(this->GetComponentSize());

  std::memcpy(img->GetScalarPointer(), buffer, static_cast<size_t>(nbytes));

  vtkNew<vtkXMLImageDataWriter> writer;
  writer->SetInputData(img);
  writer->SetFileName(m_FileName.c_str());
  writer->Write();
}
