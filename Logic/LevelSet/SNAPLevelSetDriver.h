/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPLevelSetDriver.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
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
#ifndef __SNAPLevelSetDriver_h_
#define __SNAPLevelSetDriver_h_

#include "SNAPCommon.h"
#include "SnakeParameters.h"
#include "SNAPLevelSetFunction.h"
// #include "SNAPLevelSetStopAndGoFilter.h"

template <class TFilter> class LevelSetExtensionFilter;
class LevelSetExtensionFilterInterface;
 
namespace itk {
  template <class TInputImage, class TOutputImage> class ImageToImageFilter;
  template <class TInputImage, class TOutputImage> class FiniteDifferenceImageFilter;
  template <class TOwner> class SimpleMemberCommand;
  template <class TOwner> class MemberCommand;
  class Command;
};

/** Accessor class to cast speed image to float or double */
/*
template <class TExternalType>
class SpeedImageToFloatAccessor
{
public:
  typedef short InternalType;
  typedef TExternalType ExternalType;

  static inline void Set(InternalType &output, const ExternalType &input)
  {
    output = static_cast<InternalType>(input * 0x7fff);
  }

  static inline ExternalType Get(const InternalType &input)
  {
    return static_cast<ExternalType>(input * (1.0 / 0x7fff));
  }
};
*/

/** 
 * \class SNAPLevelSetDriverBase
 * \brief An abstract interface that allows code to be written independently of
 * the dimensionality of the level set filter. For documentation of the methods,
 * see SNAPLevelSetDriver.
 */
class SNAPLevelSetDriverBase
{
public:
    virtual ~SNAPLevelSetDriverBase() { /*To avoid compiler warning.*/ }
  /** Set snake parameters */
  virtual void SetSnakeParameters(const SnakeParameters &parms) = 0;

  /** Run the snake for a number of iterations */
  virtual void Run(unsigned int nIterations) = 0;

  /** Restart the snake */
  virtual void Restart() = 0;

  /** Clean up the snake's state */
  virtual void CleanUp() = 0;
};

/**
 * \class SNAPLevelSetDriver
 * \brief A generic interface between the SNAP application and ITK level set
 * framework.
 *
 * This interface allows the SNAP code to exist independently of the way stop-and-go
 * level set evolution is implemented in ITK.  This gives the software a bit of 
 * modularity.  As far as SNAP cares, the public methods declared in this class are
 * the only ways to control level set evolution.
 */
template <unsigned int VDimension> 
class SNAPLevelSetDriver : public SNAPLevelSetDriverBase
{
public:

  typedef SNAPLevelSetDriver                                           Self;

  // A callback type
  typedef itk::SmartPointer<itk::Command>                    CommandPointer;
  typedef itk::SimpleMemberCommand<Self>                    SelfCommandType;
  typedef itk::SmartPointer<SelfCommandType>             SelfCommandPointer;

  /** Speed image type: short, to conserve memory */
  typedef itk::Image<short, VDimension>              ShortImageType;

  /** Floating point image type used internally */
  typedef itk::Image<float, VDimension>              FloatImageType;
  typedef typename itk::SmartPointer<FloatImageType>      FloatImagePointer;

  /** Type definition for the level set function */
  typedef SNAPLevelSetFunction<ShortImageType, FloatImageType>
                                                       LevelSetFunctionType;
  typedef typename LevelSetFunctionType::VectorImageType    VectorImageType;

  /** Initialize the level set driver.  Note that the type of snake (in/out
   * or edge) is determined entirely by the speed image and by the values
   * of the parameters.  Moreover, the type of solver used is specified in
   * the parameters as well. The last parameter is the optional external 
   * advection field, that can be used instead of the default advection
   * field that is based on the image gradient */
  SNAPLevelSetDriver(FloatImageType *initialLevelSet,
                     ShortImageType *speed,
                     const SnakeParameters &parms,
                     VectorImageType *externalAdvection = NULL);

  /** Virtual destructor */
  virtual ~SNAPLevelSetDriver() {}

  /** Set snake parameters */
  void SetSnakeParameters(const SnakeParameters &parms);

  /** Run the filter */
  void Run(unsigned int nIterations);

  /** Check for convergence */
  bool IsEvolutionConverged();

  /** Restart the snake */
  void Restart();

  /** Get the level set function */
  irisGetMacro(LevelSetFunction,LevelSetFunctionType *);

  /** Get the current state of the snake (level set and narrow band) */
  FloatImageType *GetCurrentState();

  /** Get the number of elapsed iterations */
  unsigned int GetElapsedIterations() const;

  /** Clean up the snake's state */
  void CleanUp();
  
private:
  /** An internal class used to invert an image */
  class InvertFunctor {
  public:
    unsigned char operator()(unsigned char input) 
      { return input == 0 ? 1 : 0; }  
  };

  /** Type definition for the level set filter */
  typedef itk::FiniteDifferenceImageFilter<FloatImageType,FloatImageType> FilterType;

  /** Level set filter wrapped by this object */
  typename FilterType::Pointer m_LevelSetFilter;

  /** Level set function used by the level set filter */
  typename LevelSetFunctionType::Pointer m_LevelSetFunction;

  /** An initialization image */
  FloatImagePointer m_InitializationImage;

  /** Speed image adaptor */
  typename ShortImageType::Pointer m_SpeedAdaptor;

  /** Last accepted snake parameters */
  SnakeParameters m_Parameters;

  /** Assign the values of snake parameters to a snake function */
  void AssignParametersToPhi(const SnakeParameters &parms, bool firstTime);

  /** Internal routines */
  void DoCreateLevelSetFilter();
};

// Type definitions
typedef SNAPLevelSetDriver<3> SNAPLevelSetDriver3d;
typedef SNAPLevelSetDriver<2> SNAPLevelSetDriver2d;

#ifndef ITK_MANUAL_INSTANTIATION
#include "SNAPLevelSetDriver.txx"
#endif

#endif // __SNAPLevelSetDriver_h_
