/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParameters.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 19:17:00 $
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
#include "SnakeParameters.h"


SnakeParameters 
SnakeParameters
::GetDefaultEdgeParameters() 
{
  SnakeParameters p;

  p.m_AutomaticTimeStep = true;
  p.m_TimeStepFactor = 1.0f;
  p.m_Ground = 5.0;

  p.m_SnakeType = EDGE_SNAKE;
  p.m_Clamp = true;

  p.m_PropagationWeight = 1.0;
  p.m_PropagationSpeedExponent = 1;

  p.m_CurvatureWeight = 0.2;
  p.m_CurvatureSpeedExponent = 0;

  p.m_LaplacianWeight = 0.0f;
  p.m_LaplacianSpeedExponent = 0;

  p.m_AdvectionWeight = 2.0;
  p.m_AdvectionSpeedExponent = 0;       

  p.m_Solver = PARALLEL_SPARSE_FIELD_SOLVER;

  return p;
}

SnakeParameters 
SnakeParameters
::GetDefaultInOutParameters() 
{
  SnakeParameters p;

  p.m_AutomaticTimeStep = true;
  p.m_TimeStepFactor = 1.0f;
  p.m_Ground = 5.0;

  p.m_SnakeType = REGION_SNAKE;
  p.m_Clamp = true;

  p.m_PropagationWeight = 1.0;
  p.m_PropagationSpeedExponent = 1;

  p.m_CurvatureWeight = 0.2;
  p.m_CurvatureSpeedExponent = -1;

  p.m_LaplacianWeight = 0.0f;
  p.m_LaplacianSpeedExponent = 0;

  p.m_AdvectionWeight = 0;
  p.m_AdvectionSpeedExponent = 0;       

  p.m_Solver = PARALLEL_SPARSE_FIELD_SOLVER;

  return p;
}

SnakeParameters 
SnakeParameters
::GetDefaultAllZeroParameters() 
{
  SnakeParameters p;

  p.m_AutomaticTimeStep = true;
  p.m_TimeStepFactor = 0.1f;
  p.m_Ground = 5.0;

  p.m_SnakeType = REGION_SNAKE;
  p.m_Clamp = true;

  p.m_PropagationWeight = 0.0;
  p.m_PropagationSpeedExponent = 0;

  p.m_CurvatureWeight = 0.0;
  p.m_CurvatureSpeedExponent = -1;

  p.m_LaplacianWeight = 0.0f;
  p.m_LaplacianSpeedExponent = 0;

  p.m_AdvectionWeight = 0;
  p.m_AdvectionSpeedExponent = 0;       

  p.m_Solver = PARALLEL_SPARSE_FIELD_SOLVER;

  return p;
}

bool 
SnakeParameters
::operator == (const SnakeParameters &p) const
{
  return(
    m_AutomaticTimeStep == p.m_AutomaticTimeStep &&
    (m_AutomaticTimeStep || (m_TimeStepFactor == p.m_TimeStepFactor)) &&
    m_Ground == p.m_Ground &&
    m_SnakeType == p.m_SnakeType &&
    m_Clamp == p.m_Clamp &&
    m_PropagationWeight == p.m_PropagationWeight &&
    m_PropagationSpeedExponent == p.m_PropagationSpeedExponent &&
    m_CurvatureWeight == p.m_CurvatureWeight &&
    m_CurvatureSpeedExponent == p.m_CurvatureSpeedExponent &&
    m_LaplacianWeight == p.m_LaplacianWeight &&
    m_LaplacianSpeedExponent == p.m_LaplacianSpeedExponent &&
    m_AdvectionWeight == p.m_AdvectionWeight &&
    m_AdvectionSpeedExponent == p.m_AdvectionSpeedExponent && 
    m_Solver == p.m_Solver);
}
