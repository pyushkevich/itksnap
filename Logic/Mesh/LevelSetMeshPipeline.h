/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LevelSetMeshPipeline.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.3 $
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
#ifndef __LevelSetMeshPipeline_h_
#define __LevelSetMeshPipeline_h_

#include "SNAPCommon.h"
#include "itkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "itkObject.h"
#include "itkObjectFactory.h"

// Forward reference to itk classes
namespace itk {
  template <class TPixel,unsigned int VDimension> class Image;
  class FastMutexLock;
}

// Forward reference to our own VTK pipeline
class MeshOptions;
class VTKMeshPipeline;
class vtkPolyData;

/**
 * \class LevelSetMeshPipeline
 * \brief A pipeline used to compute a mesh of the zero level set in SNAP.
 *
 * This pipeline takes a floating point image computed by the level
 * set filter and uses a contour algorithm to get a triangular mesh
 */
class LevelSetMeshPipeline : public itk::Object
{
public:  

  irisITKObjectMacro(LevelSetMeshPipeline, itk::Object)

  /** Input image type */
  typedef itk::Image<float,3> InputImageType;
  typedef itk::SmartPointer<InputImageType> InputImagePointer;

  /** Set the input segmentation image */
  void SetImage(InputImageType *input);

  /** Set the mesh options for this filter */
  void SetMeshOptions(const MeshOptions *options);

  /** Compute the mesh for the segmentation level set. An optional pointer
      to a mutex lock can be provided. If passed in, the portion of the code
      where the image data is accessed will be locked. This is to prevent mesh
      update clashing with level set evolution iteration. */
  void UpdateMesh(itk::FastMutexLock *lock = NULL);

  /** Get the stored mesh */
  vtkPolyData *GetMesh();

protected:
  
  /** Constructor, which builds the pipeline */
  LevelSetMeshPipeline();

  /** Deallocate the pipeline filters */
  ~LevelSetMeshPipeline();

private:
  // Type definitions for the various filters used by this object
  typedef InputImageType InternalImageType;
  typedef itk::SmartPointer<InternalImageType> InternalImagePointer;
  
  // Current set of mesh options
  SmartPtr<MeshOptions> m_MeshOptions;

  // The input image
  InputImagePointer m_InputImage;

  // The VTK pipeline
  VTKMeshPipeline *m_VTKPipeline;

  // The output mesh
  vtkSmartPointer<vtkPolyData> m_Mesh;
};

#endif //__LevelSetMeshPipeline_h_
