/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MeshManager.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#ifndef __MeshManager_h_
#define __MeshManager_h_

// Forward references
class IRISApplication;
class GlobalState;
class IRISImageData;
class ColorLabel;
class AllPurposeProgressAccumulator;
class vtkPolyData;
class MultiLabelMeshPipeline;
class LevelSetMeshPipeline;


#include "SNAPCommon.h"
#include "AllPurposeProgressAccumulator.h"
#include <vector>
#include "itkObject.h"
#include "vtkSmartPointer.h"

namespace itk {
    template<unsigned int VImageDimension>
        class ImageBase;
}

/**
 * \class MeshManager
 * \brief A class representing a mesh generated from a segmentation.
 *
 * This class wraps around MultiLabelMeshPipeline and LevelSetMeshPipeline.  It's a very
 * high level class that generates a correct mesh based on the current state of the 
 * application.
 */
class MeshManager : public itk::Object
{
public:
  irisITKObjectMacro(MeshManager, itk::Object)

  /**
   * Initialize the object with an IRISApplication pointer
   */
  void Initialize(IRISApplication *driver);

  /**
   * Generate VTK meshes from input data.
   */
  void UpdateVTKMeshes(itk::Command *command);

  /**
   * Get the mapping of labels to vtk mesh pointers. This method has a
   * slight overhead of copying the data
   */
  typedef std::map<LabelType, vtkSmartPointer<vtkPolyData> > MeshCollection;
  MeshCollection GetMeshes();

  /**
   * Does the mesh need updating?
   */
  bool IsMeshDirty();

  /**
    Get the time that the mesh was built
    */
  irisGetMacro(BuildTime, unsigned long)

protected:

  MeshManager();
  virtual ~MeshManager();

  /** 
   * This method applies the settings in a color label if color label 
   * is displayable
   */
  bool ApplyColorLabel(const ColorLabel &label);

  // Back pointer to the application object
  IRISApplication *m_Driver;

  // Pointer to the global state object
  GlobalState *m_GlobalState;

  // Progress accumulator for multi-object rendering
  itk::SmartPointer<AllPurposeProgressAccumulator> m_Progress;

  // Time when the mesh was last build
  unsigned long m_BuildTime;

  //Check if apImage is a proper 3D, i.e. the third dimension is
  //different than 1
  bool Is3DProper(const itk::ImageBase<3> * apImage) const;

};

#endif // __MeshManager_h_

/*
 *$Log: MeshManager.h,v $
 *Revision 1.3  2007/12/30 04:05:15  pyushkevich
 *GPL License
 *
 *Revision 1.2  2007/05/10 20:19:50  pyushkevich
 *Added VTK mesh export code and GUI
 *
 *Revision 1.1  2006/12/02 04:22:15  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.8  2005/10/29 14:00:14  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.7  2003/10/09 22:45:13  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.6  2003/10/02 14:54:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:50:29  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.4  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.3  2003/08/27 14:03:21  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:46  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:52:25  pauly
 *Initial checkin of SNAP application  to the InsightApplications tree
 *
 *Revision 1.7  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.6  2003/07/11 23:29:17  pauly
 **** empty log message ***
 *
 *Revision 1.5  2003/07/11 21:25:12  pauly
 *Code cleanup for ITK checkin
 *
 *Revision 1.4  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.3  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.4  2002/04/27 17:49:00  bobkov
 *Added comments
 *
 *Revision 1.3  2002/04/11 23:09:15  bobkov
 *Commented GenerateMesh() method
 *
 *Revision 1.2  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/
