/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MeshManager.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.4 $
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
#include "MeshManager.h"

#include "SNAPOpenGL.h"

// SNAP Includes
#include "ColorLabel.h"
#include "GlobalState.h"
#include "ImageWrapper.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "MultiLabelMeshPipeline.h"
#include "LevelSetMeshPipeline.h"
#include "IRISVectorTypesToITKConversion.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "AllPurposeProgressAccumulator.h"
#include "MeshOptions.h"

// ITK includes
#include "itkRegionOfInterestImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkRecursiveGaussianImageFilter.h"
#include "itkVTKImageExport.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkDecimatePro.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkContourFilter.h>
#include <vtkImageThreshold.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkStripper.h>

// System includes
#include <cstdlib>

using namespace std;

MeshManager
::MeshManager()
{
  m_Progress = AllPurposeProgressAccumulator::New();
  m_BuildTime = 0;
}

MeshManager
::~MeshManager()
{
}

void 
MeshManager
::Initialize(IRISApplication *driver)
{
  m_Driver = driver;
  m_GlobalState = m_Driver->GetGlobalState();  
}

void 
MeshManager
::UpdateVTKMeshes(itk::Command *command)
{
  if(!m_Driver->GetCurrentImageData()->IsSegmentationLoaded())
    return;

  // The mesh is constructed differently depending on whether there is an
  // actively evolving level set or not SNAP mode or in IRIS mode
  if (m_Driver->IsSnakeModeLevelSetActive())
    {    
    // Get the mesh pipeline associated with the level set image wrapper
    LevelSetImageWrapper *wrapper = m_Driver->GetSNAPImageData()->GetSnake();
    SmartPtr<LevelSetMeshPipeline> pipeline =
        static_cast<LevelSetMeshPipeline *>(wrapper->GetUserData("MeshPipeline"));

    // TODO: this feels kind of awkward here
    if(!wrapper->GetImage() || !Is3DProper(wrapper->GetImage()))
      return;

    // If the pipeline does not exist, create it
    if(!pipeline)
      {
      pipeline = LevelSetMeshPipeline::New();
      wrapper->SetUserData("MeshPipeline", pipeline);
      }

    // Make sure the pipeline has the right image
    pipeline->SetImage(wrapper->GetImage());

    // Make sure the pipeline has the right options
    pipeline->SetMeshOptions(m_GlobalState->GetMeshOptions());

    // Compute the mesh only for the current segmentation color
    pipeline->UpdateMesh(m_Driver->GetSNAPImageData()->GetLevelSetPipelineMutexLock());
    }
  else
    {
    // Get the mesh pipeline associated with the level set image wrapper
    LabelImageWrapper *wrapper = m_Driver->GetCurrentImageData()->GetSegmentation();
    SmartPtr<MultiLabelMeshPipeline> pipeline =
        static_cast<MultiLabelMeshPipeline *>(wrapper->GetUserData("MeshPipeline"));

    // TODO: this feels kind of awkward here
    if(!wrapper->GetImage() || !Is3DProper(wrapper->GetImage()))
      return;

    // If the pipeline does not exist, create it
    if(!pipeline)
      {
      pipeline = MultiLabelMeshPipeline::New();
      wrapper->SetUserData("MeshPipeline", pipeline);
      }

    // Make sure the pipeline has the right image
    pipeline->SetImage(wrapper->GetImage());

      // Pass the options to the pipeline
    pipeline->SetMeshOptions(m_GlobalState->GetMeshOptions());

    // Update the meshes
    pipeline->UpdateMeshes(command);
    }

  // Invoke a modified event (?)
  this->Modified();

  // Record the time that the mesh was built
  m_BuildTime = this->GetMTime();
}

MeshManager::MeshCollection MeshManager::GetMeshes()
{
  MeshCollection meshes;

  if (m_Driver->IsSnakeModeLevelSetActive())
    {
    // Get the mesh pipeline associated with the level set image wrapper
    LevelSetImageWrapper *wrapper = m_Driver->GetSNAPImageData()->GetSnake();
    SmartPtr<LevelSetMeshPipeline> pipeline =
        static_cast<LevelSetMeshPipeline *>(wrapper->GetUserData("MeshPipeline"));

    // TODO: this feels kind of awkward here
    if(!wrapper->GetImage() || !Is3DProper(wrapper->GetImage()))
      return meshes;

    if(pipeline)
      {
      meshes[m_Driver->GetGlobalState()->GetDrawingColorLabel()] = pipeline->GetMesh();
      return meshes;
      }
    }
  else if(m_Driver->GetCurrentImageData()->IsSegmentationLoaded())
    {
    // Get the mesh pipeline associated with the level set image wrapper
    LabelImageWrapper *wrapper = m_Driver->GetCurrentImageData()->GetSegmentation();
    SmartPtr<MultiLabelMeshPipeline> pipeline =
        static_cast<MultiLabelMeshPipeline *>(wrapper->GetUserData("MeshPipeline"));

    // TODO: this feels kind of awkward here
    if(!wrapper->GetImage() || !Is3DProper(wrapper->GetImage()))
      return meshes;

    if(pipeline)
      {
      return pipeline->GetMeshCollection();
      }
    }

  return meshes;
}

bool MeshManager::IsMeshDirty()
{
  // If there is no image loaded, the mesh is not considered dirty
  if(!m_Driver->IsMainImageLoaded())
    return false;

  // Image base
  itk::ImageBase<3> *image = NULL;

  // Get the appropriate source image
  if(m_Driver->IsSnakeModeLevelSetActive())
    {
    // We are in SNAP.  Use one of SNAP's images
    SNAPImageData *snapData = m_Driver->GetSNAPImageData();
    image = snapData->GetSnake()->GetImage();
    }
  else
    {
    image = m_Driver->GetCurrentImageData()->GetSegmentation()->GetImageBase();
    }

  // Compare the timestamps
  if(image->GetMTime() > this->m_BuildTime)
    return true;

  // Also check if the mesh options have been modified since
  if(m_Driver->GetGlobalState()->GetMeshOptions()->GetMTime() > this->m_BuildTime)
    return true;

  return false;
}


/*
 *  Apply color label, a shorthand
 */
bool 
MeshManager
::ApplyColorLabel(const ColorLabel &label) 
{
  if (label.IsVisible() && label.IsVisibleIn3D()) 
    {
    // Adjust the label color to reduce the saturation. This in necessary
    // in order to see the highlights on the object
    double r = 0.75 * (label.GetRGB(0) / 255.0);
    double g = 0.75 * (label.GetRGB(1) / 255.0);
    double b = 0.75 * (label.GetRGB(2) / 255.0);
    double a = label.GetAlpha() / 255.0;

    if (label.IsOpaque()) glColor3d(r, g, b); 
    else glColor4d(r, g, b, a);
  
    return true;
    }
  return false;
}


bool MeshManager
::Is3DProper(const itk::ImageBase<3> * apImage) const {
    
    const itk::ImageBase<3>::RegionType region = apImage->GetLargestPossibleRegion();
    itk::ImageBase<3>::SizeType size = region.GetSize();
    if((size[0] <= 1) || (size[0] <= 1) || (size[2] <= 1))
        return(false);
    return(true);
}

/*
 *$Log: MeshManager.cxx,v $
 *Revision 1.4  2010/06/28 18:45:08  pyushkevich
 *Patch from Michael Hanke to allow ITK 3.18 builds
 *
 *Revision 1.3  2007/12/30 04:05:15  pyushkevich
 *GPL License
 *
 *Revision 1.2  2007/05/10 20:19:50  pyushkevich
 *Added VTK mesh export code and GUI
 *
 *Revision 1.1  2006/12/02 04:22:14  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.19  2005/12/08 18:20:45  hjohnson
 *COMP:  Removed compiler warnings from SGI/linux/MacOSX compilers.
 *
 *Revision 1.18  2005/11/23 14:32:15  ibanez
 *BUG: 2404. Patch provided by Paul Yushkevish.
 *
 *Revision 1.17  2005/10/29 14:00:14  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.16  2005/04/21 14:46:29  pauly
 *ENH: Improved management and editing of color labels in SNAP
 *
 *Revision 1.15  2004/09/14 14:11:09  pauly
 *ENH: Added an activation manager to main UI class, improved snake code, various UI fixes and additions
 *
 *Revision 1.14  2004/08/26 19:43:23  pauly
 *ENH: Moved the Borland code into Common folder
 *
 *Revision 1.13  2004/08/03 23:26:32  ibanez
 *ENH: Modification for building in multple platforms. By Julien Jomier.
 *
 *Revision 1.12  2004/07/29 14:01:56  pauly
 *ENH: An interface for changing SNAP appearance settings
 *
 *Revision 1.11  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.10  2004/01/20 00:17:42  pauly
 *FIX: VTK float compatibility
 *
 *Revision 1.9  2004/01/17 18:39:07  lorensen
 *ENH: changes to accomodate VTK api changes.
 *
 *Revision 1.8  2003/10/09 22:45:13  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.7  2003/10/02 14:54:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:50:29  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.5  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.4  2003/08/27 14:03:21  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.3  2003/08/27 04:57:46  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.2  2003/08/14 13:37:07  pauly
 *FIX: Error with using int instead of vtkIdType in calling vtkCellArray::GetNextCell
 *
 *Revision 1.1  2003/07/12 04:52:25  pauly
 *Initial checkin of SNAP application  to the InsightApplications tree
 *
 *Revision 1.2  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.1  2003/07/11 23:29:17  pauly
 **** empty log message ***
 *
 *Revision 1.10  2003/07/11 21:25:12  pauly
 *Code cleanup for ITK checkin
 *
 *Revision 1.9  2003/07/10 14:30:26  pauly
 *Integrated ITK into SNAP level set segmentation
 *
 *Revision 1.8  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.7  2003/05/12 02:51:10  pauly
 *Got code to compile on UNIX
 *
 *Revision 1.6  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.5  2003/04/29 14:01:42  pauly
 *Charlotte Trip
 *
 *Revision 1.4  2003/04/23 20:36:23  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.11  2002/05/08 17:34:18  moon
 *Fixed bug of using the voxel snake image, instead of hte float snake state,
 *for the marching cubes. Now the snake looks smooth instead of voxelized.
 *
 *Revision 1.10  2002/04/27 18:30:22  moon
 *Finished commenting
 *
 *Revision 1.9  2002/04/27 17:49:00  bobkov
 *Added comments
 *
 *Revision 1.8  2002/04/24 17:12:48  bobkov
 *modified display() method so that when num_lists is zero
 *the 3D window does not show anything
 *
 *Revision 1.7  2002/04/23 19:30:10  bobkov
 *modified display method so that the current color label is
 *used in the snake 3d window
 *
 *Revision 1.6  2002/04/13 17:45:32  moon
 *I put code in to check if a label exists in the seg image in GenerateMesh,
 *so that the vtk pipeline (SLOW!) wouldn't have to get executed on empty images.
 *(i.e. all 6 or whatever labels that are initialized at the start are "rendered"
 *even if only one label has actually been used)
 *
 *Revision 1.5  2002/04/10 21:20:42  moon
 *fixed bug when update mesh is pressed with no seg data
 *
 *Revision 1.4  2002/04/10 20:20:36  moon
 *fixed small problem with snake 3d window mesh generation with Konstantin
 *It was using full_data, and we switched it to use only roi_data.
 *
 *Revision 1.3  2002/04/09 22:00:23  bobkov
 *
 *Modified GenerateMesh method to display 3d snake segmentation
 *
 *Revision 1.2  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/
