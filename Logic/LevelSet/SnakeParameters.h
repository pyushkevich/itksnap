/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParameters.h,v $
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
#ifndef __SnakeParameters_h_
#define __SnakeParameters_h_

#include "SNAPCommon.h"

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
  irisGetMacro(AutomaticTimeStep,bool);
  irisSetMacro(AutomaticTimeStep,bool);

  /** Time step factor in level snake propagation.  This is is only used if the
   * automatic computation is off, and represents the factor by which the auto
   * time step is multiplied */
  irisGetMacro(TimeStepFactor,float);
  irisSetMacro(TimeStepFactor,float);

  /** Clamp-to-ground parameter.  Obsolete in ITK implementation, kept for
    backward compatibility and regression testing */
  irisGetMacro(Ground,float);
  irisSetMacro(Ground,float);

  /** Whether to clamp or not.  Obsolete in ITK implementation, kept for
    backward compatibility and regression testing */
  irisGetMacro(Clamp,bool);
  irisSetMacro(Clamp,bool);

  /** Which solver to use to run the equation */
  irisGetMacro(Solver,SolverType);
  irisSetMacro(Solver,SolverType);

  /** Type of equation (well known parameter sets) */
  irisGetMacro(SnakeType,SnakeType);
  irisSetMacro(SnakeType,SnakeType);

  irisGetMacro(PropagationWeight,float);
  irisSetMacro(PropagationWeight,float);
  
  irisGetMacro(PropagationSpeedExponent,int);
  irisSetMacro(PropagationSpeedExponent,int);

  irisGetMacro(CurvatureWeight,float);
  irisSetMacro(CurvatureWeight,float);

  irisGetMacro(CurvatureSpeedExponent,int);
  irisSetMacro(CurvatureSpeedExponent,int);

  irisGetMacro(LaplacianWeight,float);
  irisSetMacro(LaplacianWeight,float);

  irisGetMacro(LaplacianSpeedExponent,int);
  irisSetMacro(LaplacianSpeedExponent,int);

  irisGetMacro(AdvectionWeight,float);
  irisSetMacro(AdvectionWeight,float);

  irisGetMacro(AdvectionSpeedExponent,int);
  irisSetMacro(AdvectionSpeedExponent,int);
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
