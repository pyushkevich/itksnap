/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: TestBase.h,v $
  Language:  C++
  Date:      $Date: 2009/11/14 16:19:56 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __TestBase_h_
#define __TestBase_h_

#include "CommandLineArgumentParser.h"
#include "ImageIORoutines.h"
#include "itkOrientedImage.h"

/** Exception when needed parameters are omitted */
class TestUsageException : public itk::ExceptionObject {};

class TestBase
{
public:
  // Describe the usage of the test
  virtual void PrintUsage() = 0;

  // Get the ID of the test for command line specification
  virtual const char *GetTestName() = 0;
  
  // Get the ID of the test for command line specification
  virtual const char *GetDescription() = 0;

  // Tack on options to the command line parser
  virtual void ConfigureCommandLineParser(CommandLineArgumentParser &parser) = 0;

  // Pass command line parameters to the test
  virtual void SetCommandLineParameters(
    const CommandLineArgumentParseResult &parameters)
    {
    m_Command = parameters;
    }

  // Create a new test with given command line parameters.  The test
  // either runs succesfully and returns, or throws an exception
  // describing what happened.
  virtual void Run() = 0;

  // Destructor
  virtual ~TestBase() {}

protected:

  // The command line parse result, i.e., parameters of the test
  CommandLineArgumentParseResult m_Command;    
};

template <class TPixel>
class TestBaseOneImage : public TestBase
{
public:
  // Type definitions
  typedef itk::OrientedImage<TPixel,3> ImageType;
  typedef typename ImageType::Pointer ImagePointer;

  // Print how to use this test
  virtual void PrintUsage()
  {
    std::cerr << "  image FILE : Pass image name " << std::endl;
  }

  // Tack on options to the command line parser
  virtual void ConfigureCommandLineParser(CommandLineArgumentParser &parser)
  {
    parser.AddOption("image",1);
  }

  // Run method, loads the passed in message
  virtual void Run()
  {
    // Check if the image option is present
    if(!m_Command.IsOptionPresent("image"))
      throw new TestUsageException();

    // Load the image - will throw an exception if something fails
    LoadImageFromFile(m_Command.GetOptionParameter("image"),m_Image);
  }

  // Destructor
  virtual ~TestBaseOneImage() {}

protected:
  // The loaded image
  ImagePointer m_Image;
};

#endif //__TestBase_h_
