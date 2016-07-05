/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParameters.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __SnakeParameters_h_
#define __SnakeParameters_h_

#include "itkMacro.h"

// TODO: implement this using AbstractPropertyContainerModel

/**
 * \class SnakeParameters
 * \brief Parameters for the Level Set evolution.
 * Most of these parameters correspond to the terms in LevelSetFunction.
 *
 * \sa itk::LevelSetFunction
 */
class SnakeParameters
{
public:
    virtual ~SnakeParameters() { /*To avoid compiler warning.*/ }
  enum ConstraintsType {
    SAPIRO, SCHLEGEL, TURELLO, USER
  };

  enum SnakeType {
    EDGE_SNAKE, REGION_SNAKE
  };

  enum SolverType {
    PARALLEL_SPARSE_FIELD_SOLVER, SPARSE_FIELD_SOLVER,
    NARROW_BAND_SOLVER, LEGACY_SOLVER, DENSE_SOLVER
  };


  /**
   * Initialize parameters with default values for snake extraction
   * in Edge images
   */
  static SnakeParameters GetDefaultEdgeParameters();

  /**
   * Initialize parameters with default values for snake extraction
   * in Inside/Outside images
   */
  static SnakeParameters GetDefaultInOutParameters();

  /**
   * Initialize parameters with default values for snake extraction
   * in Inside/Outside images
   */
  static SnakeParameters GetDefaultAllZeroParameters();

  // Define a comparison operator
  bool operator ==(const SnakeParameters &p) const;
  bool operator !=(const SnakeParameters &p) const
    { return !((*this) == p); }

  /** Whether we wish to automatically compute optimal time step 
   * in level snake propagation */
  itkGetConstMacro(AutomaticTimeStep,bool);
  void SetAutomaticTimeStep( bool value )
  {
    this->m_AutomaticTimeStep = value;
  }

  /** Time step factor in level snake propagation.  This is is only used if the
   * automatic computation is off, and represents the factor by which the auto
   * time step is multiplied */
  itkGetConstMacro(TimeStepFactor,float);
  void SetTimeStepFactor( float value )
  {
    this->m_TimeStepFactor = value;
  }

  /** Clamp-to-ground parameter.  Obsolete in ITK implementation, kept for
    backward compatibility and regression testing */
  itkGetConstMacro(Ground,float);
  void SetGround( float value )
  {
    this->m_Ground = value;
  }

  /** Whether to clamp or not.  Obsolete in ITK implementation, kept for
    backward compatibility and regression testing */
  itkGetConstMacro(Clamp,bool);
  void SetClamp( bool value )
  {
    this->m_Clamp = value;
  }

  /** Which solver to use to run the equation */
  itkGetConstMacro(Solver,SolverType);
  void SetSolver( SolverType value )
  {
    this->m_Solver = value;
  }

  /** Type of equation (well known parameter sets) */
  itkGetConstMacro(SnakeType,SnakeType);
  void SetSnakeType( SnakeType value )
  {
    this->m_SnakeType = value;
  }

  itkGetConstMacro(PropagationWeight,float);
  void SetPropagationWeight( float value )
  {
    this->m_PropagationWeight = value;
  }
  
  itkGetConstMacro(PropagationSpeedExponent,int);
  void SetPropagationSpeedExponent( int value )
  {
    this->m_PropagationSpeedExponent = value;
  }

  itkGetConstMacro(CurvatureWeight,float);
  void SetCurvatureWeight( float value )
  {
    this->m_CurvatureWeight = value;
  }

  itkGetConstMacro(CurvatureSpeedExponent,int);
  void SetCurvatureSpeedExponent( int value )
  {
    this->m_CurvatureSpeedExponent = value;
  }

  itkGetConstMacro(LaplacianWeight,float);
  void SetLaplacianWeight( float value )
  {
    this->m_LaplacianWeight = value;
  }

  itkGetConstMacro(LaplacianSpeedExponent,int);
  void SetLaplacianSpeedExponent( int value )
  {
    this->m_LaplacianSpeedExponent = value;
  }

  itkGetConstMacro(AdvectionWeight,float);
  void SetAdvectionWeight( float value )
  {
    this->m_AdvectionWeight = value;
  }

  itkGetConstMacro(AdvectionSpeedExponent,int);
  void SetAdvectionSpeedExponent( int value )
  {
    this->m_AdvectionSpeedExponent = value;
  }

private:
  float m_TimeStepFactor;
  float m_Ground;

  SnakeType m_SnakeType;
  bool m_Clamp;

  bool m_AutomaticTimeStep;

  float m_PropagationWeight;
  int m_PropagationSpeedExponent;

  float m_CurvatureWeight;
  int m_CurvatureSpeedExponent;

  float m_LaplacianWeight;
  int m_LaplacianSpeedExponent;

  float m_AdvectionWeight;
  int m_AdvectionSpeedExponent;   

  SolverType m_Solver;
};

#endif // __SnakeParameters_h_
