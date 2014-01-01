/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LevelSetMeshPipeline.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "LevelSetMeshPipeline.h"
#include "VTKMeshPipeline.h"
#include "MeshOptions.h"

LevelSetMeshPipeline
::LevelSetMeshPipeline()
{
  // Initialize the VTK Exporter
  m_VTKPipeline = new VTKMeshPipeline();

  // Create the mesh options
  m_MeshOptions = MeshOptions::New();
  m_MeshOptions->SetUseGaussianSmoothing(false);
  m_VTKPipeline->SetMeshOptions(m_MeshOptions);
}

LevelSetMeshPipeline
::~LevelSetMeshPipeline()
{
  delete m_VTKPipeline;
}

void
LevelSetMeshPipeline
::SetMeshOptions(const MeshOptions *options)
{
  if(*m_MeshOptions != *options)
    {
    // Copy the options
    m_MeshOptions->DeepCopy(options);

    // Turn of the Gaussian smoothing
    m_MeshOptions->SetUseGaussianSmoothing(false);

    // Apply the options to the internal pipeline
    m_VTKPipeline->SetMeshOptions(m_MeshOptions);
    }
}

void
LevelSetMeshPipeline
::UpdateMesh(itk::FastMutexLock *lock)
{
  // We need to generate a new mesh object. Otherwise, if there is concurrent
  // rendering and mesh computation, the mesh would be accessed by two threads
  // at the same time, which is a problem.
  m_Mesh = vtkSmartPointer<vtkPolyData>::New();

  // Run the pipeline
  m_VTKPipeline->ComputeMesh(m_Mesh, lock);
}

vtkPolyData *LevelSetMeshPipeline::GetMesh()
{
  return m_Mesh;
}

void 
LevelSetMeshPipeline
::SetImage(InputImageType *image)
{
  // Hook the input into the pipeline
  m_VTKPipeline->SetImage(image);
}

