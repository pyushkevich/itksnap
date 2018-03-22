/*=========================================================================

  Program:   ITK-SNAP
  Module:    AffineTransformWrapper.cxx
  Language:  C++
  Copyright (c) 2018 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "AffineTransformHelper.h"
#include "itkMatrixOffsetTransformBase.h"
#include "itkIdentityTransform.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFactory.h"
#include "IRISException.h"
#include "Registry.h"

void AffineTransformHelper::GetMatrixAndOffset(
    const ITKTransformBase *t, ITKMatrix &mat, ITKVector &off)
{
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;
  const TransformBase *tb = dynamic_cast<const TransformBase *>(t);

  mat.SetIdentity();
  off.Fill(0.0);
  if(tb)
    {
    mat = tb->GetMatrix();
    off = tb->GetOffset();
    }
}

bool AffineTransformHelper::IsIdentity(const ITKTransformBase *t, double tol)
{
  // Get matrix and offset
  ITKMatrix matrix;
  ITKVector offset;
  GetMatrixAndOffset(t, matrix, offset);

  // Check if transform is identity.
  bool is_identity = true;

  for(int i = 0; i < 3; i++)
    {
    if(fabs(offset[i]) > tol)
      is_identity = false;
    for(int j = 0; j < 3; j++)
      {
      if(fabs(matrix(i,j) - (i==j ? 1.0 : 0.0)) > tol)
        is_identity = false;
      }
    }

  return is_identity;
}




void AffineTransformHelper::WriteAsITKTransform(const ITKTransformBase *t, const char *file)
{
  // Cast to transform with matrix and offset
  SmartPtr<const ITKTransformMOTB> tmo = CastToMOTB(t);

  itk::TransformFileWriter::Pointer writer = itk::TransformFileWriter::New();
  writer->SetFileName(file);
  writer->SetInput(tmo);
  writer->Update();
}

void
AffineTransformHelper
::WriteAsRASMatrix(const ITKTransformBase *t, const char *file)
{
  Mat44 Q = GetRASMatrix(t);

  // Write the matrix
  std::ofstream matrixFile;
  matrixFile.open(file);
  matrixFile << Q;
  matrixFile.close();
}

SmartPtr<AffineTransformHelper::ITKTransformMOTB>
AffineTransformHelper::ReadAsITKTransform(const char *file)
{
  // Create a factory for reading base transforms
  itk::TransformFactory<ITKTransformMOTB>::RegisterTransform();

  // Read the trasnform
  itk::TransformFileReader::Pointer reader = itk::TransformFileReader::New();
  reader->SetFileName(file);
  reader->Update();

  // Get the transform pointer
  SmartPtr<ITKTransformMOTB> tran;
  if(reader->GetTransformList()->size())
    tran = dynamic_cast<ITKTransformMOTB *>(reader->GetTransformList()->front().GetPointer());

  if(tran.IsNull())
    throw IRISException("Failed to read linear transform from file %s", file);

  return tran;
}

SmartPtr<AffineTransformHelper::ITKTransformMOTB>
AffineTransformHelper::ReadAsRASMatrix(const char *file)
{
  // Read the matrix file
  vnl_matrix<double> Q(4, 4);
  std::ifstream fin(file);
  for(size_t i = 0; i < 4; i++)
    for(size_t j = 0; j < 4; j++)
      if(fin.good())
        {
        fin >> Q[i][j];
        }
      else
        {
        fin.close();
        throw IRISException("Unable to read 4x4 matrix from file %s", file);
        }
  fin.close();

  // Flip between RAS and LPS
  Q(2,0) *= -1; Q(2,1) *= -1;
  Q(0,2) *= -1; Q(1,2) *= -1;
  Q(0,3) *= -1; Q(1,3) *= -1;

  // Populate the matrix and offset components
  ITKMatrix matrix;
  ITKVector offset;
  matrix.GetVnlMatrix() = Q.extract(3, 3, 0, 0);
  offset.SetVnlVector(Q.get_column(3).extract(3, 0));

  // Create the transform
  SmartPtr<ITKTransformMOTB> tran = ITKTransformMOTB::New();
  tran->SetMatrix(matrix);
  tran->SetOffset(offset);

  return tran;
}

SmartPtr<AffineTransformHelper::ITKTransformBase>
AffineTransformHelper::ReadFromRegistry(Registry *reg)
{
  typedef itk::IdentityTransform<double, 3> IdTransform;
  SmartPtr<IdTransform> id_transform = IdTransform::New();
  SmartPtr<ITKTransformBase> transform = id_transform.GetPointer();

  // No registry? Return identity transform
  if(!reg)
    return transform;

  // Is the transform identity
  Registry &folder = reg->Folder("ImageTransform");
  if(folder["IsIdentity"][true])
    return transform;

  // Read the matrix, defaulting to identity
  ITKMatrix matrix; matrix.SetIdentity();
  ITKVector offset; offset.Fill(0.0);

  for(int i = 0; i < 3; i++)
    {
    offset[i] = folder[folder.Key("Offset.Element[%d]",i)][offset[i]];
    for(int j = 0; j < 3; j++)
      matrix(i, j) = folder[folder.Key("Matrix.Element[%d][%d]",i,j)][matrix(i,j)];
    }

  // Check the matrix and offset for being identity
  if(matrix.GetVnlMatrix().is_identity() && offset.GetVnlVector().is_zero())
    return transform;

  // Use the matrix/offset transform
  SmartPtr<ITKTransformMOTB> motb = ITKTransformMOTB::New();
  motb->SetMatrix(matrix);
  motb->SetOffset(offset);
  transform = motb.GetPointer();
  return transform;
}

void AffineTransformHelper
::WriteToRegistry(Registry *reg, const ITKTransformBase *t)
{
  // Get the target folder
  Registry &folder = reg->Folder("ImageTransform");

  // Cast the transform to the matrix/offset type
  SmartPtr<const ITKTransformMOTB> motb = CastToMOTB(t);

  // Check for identity
  if(!motb || (motb->GetMatrix().GetVnlMatrix().is_identity() &&
               motb->GetOffset().GetVnlVector().is_zero()))
    {
    folder["IsIdentity"] << true;
    folder.RemoveKeys("Matrix");
    folder.RemoveKeys("Offset");
    }
  else
    {
    folder["IsIdentity"] << false;

    for(int i = 0; i < 3; i++)
      {
      folder[folder.Key("Offset.Element[%d]",i)] << motb->GetOffset()[i];
      for(int j = 0; j < 3; j++)
        folder[folder.Key("Matrix.Element[%d][%d]",i,j)] << motb->GetMatrix()(i,j);
      }
    }
}

AffineTransformHelper::Mat44
AffineTransformHelper::GetRASMatrix(const ITKTransformBase *t)
{
  // Get matrix and offset
  ITKMatrix matrix;
  ITKVector offset;
  GetMatrixAndOffset(t, matrix, offset);

  // Create a 4x4 matrix in RAS coordinate space
  vnl_matrix<double> Q(4, 4);
  vnl_matrix<double> v(offset.GetVnlVector().data_block(), 3, 1);
  Q.set_identity();
  Q.update(matrix.GetVnlMatrix(), 0, 0);
  Q.update(v, 0, 3);

  // Flip between RAS and LPS
  Q(2,0) *= -1; Q(2,1) *= -1;
  Q(0,2) *= -1; Q(1,2) *= -1;
  Q(0,3) *= -1; Q(1,3) *= -1;

  // Return  Q
  return Q;
}

SmartPtr<const AffineTransformHelper::ITKTransformMOTB>
AffineTransformHelper::CastToMOTB(const ITKTransformBase *t)
{
  ITKTransformMOTB::ConstPointer p = dynamic_cast<const ITKTransformMOTB *>(t);
  if(p.IsNull())
    {
    ITKMatrix matrix; matrix.SetIdentity();
    ITKVector offset; offset.Fill(0.0);
    SmartPtr<ITKTransformMOTB> p1 = ITKTransformMOTB::New();
    p1->SetMatrix(matrix);
    p1->SetOffset(offset);
    p = p1;
    }
  return p;
}
