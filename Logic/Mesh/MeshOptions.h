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
#include "AbstractPropertyContainerModel.h"

/**
 * \class MeshOptions
 * \brief A set of options for mesh display in IRIS.
 */
class MeshOptions : public AbstractPropertyContainerModel
{
public:

  irisITKObjectMacro(MeshOptions, AbstractModel)

  // Gaussian smoothing properties
  irisSimplePropertyAccessMacro(UseGaussianSmoothing,bool)
  irisRangedPropertyAccessMacro(GaussianStandardDeviation,float)
  irisRangedPropertyAccessMacro(GaussianError,float)
  irisSimplePropertyAccessMacro(UseDecimation,bool)

  // Decimation properties
  irisRangedPropertyAccessMacro(DecimateTargetReduction,float)
  irisRangedPropertyAccessMacro(DecimateMaximumError,float)
  irisRangedPropertyAccessMacro(DecimateFeatureAngle,float)
  irisSimplePropertyAccessMacro(DecimatePreserveTopology,bool)

  // Mesh smoothing properties
  irisSimplePropertyAccessMacro(UseMeshSmoothing,bool)
  irisRangedPropertyAccessMacro(MeshSmoothingRelaxationFactor,float)
  irisRangedPropertyAccessMacro(MeshSmoothingIterations,unsigned int)
  irisRangedPropertyAccessMacro(MeshSmoothingConvergence,float)
  irisRangedPropertyAccessMacro(MeshSmoothingFeatureAngle,float)
  irisSimplePropertyAccessMacro(MeshSmoothingFeatureEdgeSmoothing,bool)
  irisSimplePropertyAccessMacro(MeshSmoothingBoundarySmoothing,bool)

protected:
  MeshOptions();

private:
  // Begin render switches
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseGaussianSmoothingModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseDecimationModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_UseMeshSmoothingModel;
  
  // Begin gsmooth params
  SmartPtr<ConcreteRangedFloatProperty> m_GaussianStandardDeviationModel;
  SmartPtr<ConcreteRangedFloatProperty> m_GaussianErrorModel;
  
  // Begin decimate parameters
  SmartPtr<ConcreteRangedFloatProperty> m_DecimateTargetReductionModel;
  SmartPtr<ConcreteRangedFloatProperty> m_DecimateMaximumErrorModel;
  SmartPtr<ConcreteRangedFloatProperty> m_DecimateFeatureAngleModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_DecimatePreserveTopologyModel;
  
  // Begin msmooth params
  SmartPtr<ConcreteRangedFloatProperty> m_MeshSmoothingRelaxationFactorModel;
  SmartPtr<ConcreteRangedUIntProperty> m_MeshSmoothingIterationsModel;
  SmartPtr<ConcreteRangedFloatProperty> m_MeshSmoothingConvergenceModel;
  SmartPtr<ConcreteRangedFloatProperty> m_MeshSmoothingFeatureAngleModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_MeshSmoothingFeatureEdgeSmoothingModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_MeshSmoothingBoundarySmoothingModel;
};

#endif // __MeshOptions_h_


