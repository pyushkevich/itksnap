#include "VTIImageIO.h"

#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkMatrix3x3.h>
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
    case VTK_CHAR:               return itk::IOComponentEnum::CHAR;
    case VTK_SIGNED_CHAR:        return itk::IOComponentEnum::CHAR;
    case VTK_UNSIGNED_CHAR:      return itk::IOComponentEnum::UCHAR;
    case VTK_SHORT:              return itk::IOComponentEnum::SHORT;
    case VTK_UNSIGNED_SHORT:     return itk::IOComponentEnum::USHORT;
    case VTK_INT:                return itk::IOComponentEnum::INT;
    case VTK_UNSIGNED_INT:       return itk::IOComponentEnum::UINT;
    case VTK_LONG:               return itk::IOComponentEnum::LONG;
    case VTK_UNSIGNED_LONG:      return itk::IOComponentEnum::ULONG;
    case VTK_LONG_LONG:          return itk::IOComponentEnum::LONGLONG;
    case VTK_UNSIGNED_LONG_LONG: return itk::IOComponentEnum::ULONGLONG;
    case VTK_FLOAT:              return itk::IOComponentEnum::FLOAT;
    case VTK_DOUBLE:             return itk::IOComponentEnum::DOUBLE;
    default:                     return itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE;
    }
}

// Returns -1 for types that cannot be stored in a .vti file
static int ITKTypeToVTK(itk::IOComponentEnum itkType)
{
  switch(itkType)
    {
    case itk::IOComponentEnum::CHAR:      return VTK_SIGNED_CHAR;
    case itk::IOComponentEnum::UCHAR:     return VTK_UNSIGNED_CHAR;
    case itk::IOComponentEnum::SHORT:     return VTK_SHORT;
    case itk::IOComponentEnum::USHORT:    return VTK_UNSIGNED_SHORT;
    case itk::IOComponentEnum::INT:       return VTK_INT;
    case itk::IOComponentEnum::UINT:      return VTK_UNSIGNED_INT;
    case itk::IOComponentEnum::LONG:      return VTK_LONG;
    case itk::IOComponentEnum::ULONG:     return VTK_UNSIGNED_LONG;
    case itk::IOComponentEnum::LONGLONG:  return VTK_LONG_LONG;
    case itk::IOComponentEnum::ULONGLONG: return VTK_UNSIGNED_LONG_LONG;
    case itk::IOComponentEnum::FLOAT:     return VTK_FLOAT;
    case itk::IOComponentEnum::DOUBLE:    return VTK_DOUBLE;
    default:                              return -1;
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

void VTIImageIO::ReadAndCacheImage()
{
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(m_FileName.c_str());
  reader->Update();

  vtkImageData *img = reader->GetOutput();
  if(reader->GetErrorCode() != 0 || !img || img->GetNumberOfPoints() == 0)
    itkExceptionMacro("Error reading VTK image file " << m_FileName);

  // The voxel data are the point scalars. Files written by other tools (e.g.
  // ParaView) may store a single array without marking it as the active
  // scalars, in which case the first point data array is used.
  vtkPointData *pd = img->GetPointData();
  if(!pd->GetScalars())
    {
    if(pd->GetNumberOfArrays() == 0 || !pd->GetArray(0))
      itkExceptionMacro("VTK image file " << m_FileName << " contains no voxel data");
    pd->SetActiveScalars(pd->GetArray(0)->GetName());
    }

  if(VTKTypeToITK(pd->GetScalars()->GetDataType()) == itk::IOComponentEnum::UNKNOWNCOMPONENTTYPE)
    itkExceptionMacro("VTK image file " << m_FileName << " has an unsupported data type ("
                      << pd->GetScalars()->GetDataTypeAsString() << ")");

  m_CachedImage = img;
}

void VTIImageIO::ReadImageInformation()
{
  this->ReadAndCacheImage();
  vtkDataArray *scalars = m_CachedImage->GetPointData()->GetScalars();

  int dims[3];
  m_CachedImage->GetDimensions(dims);

  double spacing[3];
  m_CachedImage->GetSpacing(spacing);

  double origin[3];
  m_CachedImage->GetOrigin(origin);

  // VTK image data has an extent that need not start at zero, and since VTK 9
  // a direction matrix. The physical position of voxel index i is
  // origin + D * (i .* spacing), with i ranging over the extent. ITK indices
  // start at zero, so the ITK origin is the position of the first voxel.
  int extent[6];
  m_CachedImage->GetExtent(extent);
  vtkMatrix3x3 *D = m_CachedImage->GetDirectionMatrix();

  this->SetNumberOfDimensions(3);
  for(int i = 0; i < 3; i++)
    {
    double first_voxel = origin[i];
    for(int j = 0; j < 3; j++)
      first_voxel += D->GetElement(i, j) * extent[2 * j] * spacing[j];

    this->SetDimensions(i, static_cast<unsigned int>(dims[i]));
    this->SetSpacing(i, spacing[i]);
    this->SetOrigin(i, first_voxel);

    // The direction of image axis i is column i of the direction matrix
    std::vector<double> dir(3);
    for(int j = 0; j < 3; j++)
      dir[j] = D->GetElement(j, i);
    this->SetDirection(i, dir);
    }

  int ncomp = scalars->GetNumberOfComponents();
  this->SetComponentType(VTKTypeToITK(scalars->GetDataType()));

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
    this->ReadAndCacheImage();

  vtkDataArray *scalars = m_CachedImage->GetPointData()->GetScalars();
  vtkIdType nbytes = scalars->GetNumberOfTuples()
    * scalars->GetNumberOfComponents()
    * scalars->GetDataTypeSize();

  // The buffer is sized from the header information, which came from the same
  // array, but check anyway rather than overrun it
  if(static_cast<itk::SizeValueType>(nbytes) != this->GetImageSizeInBytes())
    itkExceptionMacro("Unexpected amount of voxel data in VTK image file " << m_FileName);

  std::memcpy(buffer, scalars->GetVoidPointer(0), static_cast<size_t>(nbytes));

  // Release the cached copy
  m_CachedImage = nullptr;
}

void VTIImageIO::WriteImageInformation()
{
  // Nothing needed; all handled in Write()
}

void VTIImageIO::Write(const void *buffer)
{
  // vtkImageData is at most 3D, so a 4D image cannot be stored
  for(unsigned int d = 3; d < this->GetNumberOfDimensions(); d++)
    if(this->GetDimensions(d) > 1)
      itkExceptionMacro("Images with more than three dimensions (e.g. 4D time series) "
                        "cannot be saved in the VTK image (.vti) format");

  int vtk_type = ITKTypeToVTK(this->GetComponentType());
  if(vtk_type < 0)
    itkExceptionMacro("Images of this data type cannot be saved in the VTK image (.vti) format");

  // A 2D image has a third dimension of size 1
  auto dim = [this](unsigned int d) {
    return d < this->GetNumberOfDimensions() ? static_cast<int>(this->GetDimensions(d)) : 1;
  };
  auto spc = [this](unsigned int d) {
    return d < this->GetNumberOfDimensions() ? this->GetSpacing(d) : 1.0;
  };
  auto org = [this](unsigned int d) {
    return d < this->GetNumberOfDimensions() ? this->GetOrigin(d) : 0.0;
  };

  vtkNew<vtkImageData> img;
  img->SetDimensions(dim(0), dim(1), dim(2));
  img->SetSpacing(spc(0), spc(1), spc(2));
  img->SetOrigin(org(0), org(1), org(2));

  // The direction of image axis i goes into column i of the direction matrix
  vtkNew<vtkMatrix3x3> D;
  D->Identity();
  for(unsigned int i = 0; i < 3 && i < this->GetNumberOfDimensions(); i++)
    {
    std::vector<double> dir = this->GetDirection(i);
    for(unsigned int j = 0; j < 3 && j < dir.size(); j++)
      D->SetElement(j, i, dir[j]);
    }
  img->SetDirectionMatrix(D);

  img->AllocateScalars(vtk_type, static_cast<int>(this->GetNumberOfComponents()));

  vtkIdType nbytes = img->GetNumberOfPoints()
    * static_cast<vtkIdType>(this->GetNumberOfComponents())
    * static_cast<vtkIdType>(this->GetComponentSize());

  std::memcpy(img->GetScalarPointer(), buffer, static_cast<size_t>(nbytes));

  vtkNew<vtkXMLImageDataWriter> writer;
  writer->SetInputData(img);
  writer->SetFileName(m_FileName.c_str());
  if(writer->Write() == 0)
    itkExceptionMacro("Error writing VTK image file " << m_FileName);
}
