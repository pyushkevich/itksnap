/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPTestDriver.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:20 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SNAPTestDriver_h_
#define __SNAPTestDriver_h_

#include "TestBase.h"

/**
 * \class SNAPTestDriver
 * \brief Class used to launch different tests
 */
class SNAPTestDriver
{
public:
  /** Run a test as determined by the command line parameters */
  void Run(int argc, char *argv[]);

private:

  /** Print the usage of the test application */
  void PrintUsage();
  
  /** Create a test of given name, or return NULL */
  static TestBase *CreateNonTemplateTest(const char *name);
  
  /** Class to create a templated test of given name, or return NULL */
  template <class TPixel> class TemplatedTestCreator 
  {
  public:
    TemplatedTestCreator(const char *name);
    TestBase *GetTest();
  private:
    TestBase *m_Test;
  };

  /** Create a test of given name, either templated or not, o/w return NULL */
  TestBase *CreateTestInstance(const char *name);

  /** Number of tests */
  static const unsigned int NUMBER_OF_TESTS;

  /** Test names */
  static const char *m_TestNames[];
  
  /** Whether tests are templated or not */
  static const bool m_TestTemplated[];
};

#endif // __SNAPTestDriver_h_


