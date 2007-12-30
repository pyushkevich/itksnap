/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MeshOptions.h,v $
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
#ifndef __MeshOptions_h_
#define __MeshOptions_h_

#include "SNAPCommon.h"

/**
 * \class MeshOptions
 * \brief A set of options for mesh display in IRIS.
 */
class MeshOptions  
{
public:
  MeshOptions();
  virtual ~MeshOptions();

  irisGetMacro(UseGaussianSmoothing,bool);
  irisSetMacro(UseGaussianSmoothing,bool);
    
  irisGetMacro(UseDecimation,bool);
  irisSetMacro(UseDecimation,bool);

  irisGetMacro(UseMeshSmoothing,bool);
  irisSetMacro(UseMeshSmoothing,bool);

  irisGetMacro(GaussianStandardDeviation,float);
  irisSetMacro(GaussianStandardDeviation,float);

  irisGetMacro(GaussianError,float);
  irisSetMacro(GaussianError,float);

  irisGetMacro(DecimateTargetReduction,float);
  irisSetMacro(DecimateTargetReduction,float);

  irisGetMacro(DecimateInitialError,float);
  irisSetMacro(DecimateInitialError,float);

  irisGetMacro(DecimateAspectRatio,float);
  irisSetMacro(DecimateAspectRatio,float);

  irisGetMacro(DecimateFeatureAngle,float);
  irisSetMacro(DecimateFeatureAngle,float);

  irisGetMacro(DecimateErrorIncrement,float);
  irisSetMacro(DecimateErrorIncrement,float);

  irisGetMacro(DecimateMaximumIterations,unsigned int);
  irisSetMacro(DecimateMaximumIterations,unsigned int);

  irisGetMacro(DecimatePreserveTopology,bool);
  irisSetMacro(DecimatePreserveTopology,bool);

  irisGetMacro(MeshSmoothingRelaxationFactor,float);
  irisSetMacro(MeshSmoothingRelaxationFactor,float);

  irisGetMacro(MeshSmoothingIterations,unsigned int);
  irisSetMacro(MeshSmoothingIterations,unsigned int);

  irisGetMacro(MeshSmoothingConvergence,float);
  irisSetMacro(MeshSmoothingConvergence,float);

  irisGetMacro(MeshSmoothingFeatureAngle,float);
  irisSetMacro(MeshSmoothingFeatureAngle,float);

  irisGetMacro(MeshSmoothingFeatureEdgeSmoothing,bool);
  irisSetMacro(MeshSmoothingFeatureEdgeSmoothing,bool);

  irisGetMacro(MeshSmoothingBoundarySmoothing,bool);
  irisSetMacro(MeshSmoothingBoundarySmoothing,bool);


private:
  // Begin render switches
  bool m_UseGaussianSmoothing;
  bool m_UseDecimation;
  bool m_UseMeshSmoothing;
  
  // Begin gsmooth params
  float m_GaussianStandardDeviation;
  float m_GaussianError;
  
  // Begin decimate parameters
  float m_DecimateTargetReduction;
  float m_DecimateInitialError;
  float m_DecimateAspectRatio;
  float m_DecimateFeatureAngle;
  float m_DecimateErrorIncrement;
  unsigned int m_DecimateMaximumIterations;
  bool m_DecimatePreserveTopology;
  
  // Begin msmooth params
  float m_MeshSmoothingRelaxationFactor;
  unsigned int m_MeshSmoothingIterations;
  float m_MeshSmoothingConvergence;
  float m_MeshSmoothingFeatureAngle;
  bool m_MeshSmoothingFeatureEdgeSmoothing;
  bool m_MeshSmoothingBoundarySmoothing;
};

#endif // __MeshOptions_h_

/*
 *$Log: MeshOptions.h,v $
 *Revision 1.2  2007/12/30 04:05:15  pyushkevich
 *GPL License
 *
 *Revision 1.1  2006/12/02 04:22:15  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.5  2003/10/09 22:45:13  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.4  2003/10/02 14:54:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:50:29  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
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
 *Revision 1.5  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.4  2003/07/11 23:29:17  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/07/11 21:25:12  pauly
 *Code cleanup for ITK checkin
 *
 *Revision 1.2  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:30  moon
 *Added Header and Log tags to all files
 **/
