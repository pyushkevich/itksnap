/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPTestDriver.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 18:09:25 $
  Version:   $Revision: 1.5 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SNAPTestDriver.h"
#include "TestImageWrapper.h"
#include "GreyImageWrapper.h"
#include "LabelImageWrapper.h"
#include "SpeedImageWrapper.h"


#include "CommandLineArgumentParser.h"
#include <iostream>
#include <iomanip>

using namespace std;

const unsigned int SNAPTestDriver::NUMBER_OF_TESTS = 4;
const char *SNAPTestDriver::m_TestNames[] = { "ImageWrapper",
  "IRISImageData","SNAPImageData","Preprocessing" };
const bool SNAPTestDriver::m_TestTemplated[] = { true, false, false, false };

void
SNAPTestDriver
::PrintUsage()
{
  std::cerr << "SNAP Test Driver Usage:" << std::endl;
  std::cerr << "  SNAPTest list" << std::endl;
  std::cerr << "or " << std::endl;
  std::cerr << "  SNAPTest help NAME " << std::endl;
  std::cerr << "or " << std::endl;
  std::cerr << "  SNAPTest test NAME [type TYPE] [options]" << std::endl;
  std::cerr << "Commands :" << std::endl;
  std::cerr << "  list                List all known tests" << std::endl;
  std::cerr << "  help NAME           Provide help on test 'NAME'" << std::endl;
  std::cerr << "  test NAME           Run test 'NAME'" << std::endl;
  std::cerr << "  type TYPE           Specify template parameter of test" << std::endl;    
  std::cerr << "                      (e.g., unsigned_short)" << std::endl;      
}

template <class TPixel> 
SNAPTestDriver::TemplatedTestCreator<TPixel>
::TemplatedTestCreator(const char *name)
{
  string strName = name;

  if(strName == "ImageWrapper")
    {
    if(typeid(TPixel) == typeid(GreyType))
      m_Test = new TestImageWrapper<GreyType, GreyImageWrapper<GreyType> >();
    else if(typeid(TPixel) == typeid(LabelType))
      m_Test = new TestImageWrapper<LabelType, LabelImageWrapper>();
    else if(typeid(TPixel) == typeid(float))
      m_Test = new TestImageWrapper<float, SpeedImageWrapper>();
    else
      m_Test = NULL;
    }
  else
    m_Test = NULL;
}

template <class TPixel> 
TestBase *
SNAPTestDriver::TemplatedTestCreator<TPixel>
::GetTest()
{
  return m_Test;
}
  
TestBase *
SNAPTestDriver
::CreateNonTemplateTest(const char *name)
{
  string strName = name;
  TestBase *test = NULL;
 
  return test;
}

TestBase *
SNAPTestDriver
::CreateTestInstance(const char *name)
{
  TestBase *test = CreateNonTemplateTest(name);
  if(!test)
    test = TemplatedTestCreator<unsigned char>(name).GetTest();
  return test;
}

void
SNAPTestDriver
::Run(int argc, char *argv[])
{
  // Configure the command line
  CommandLineArgumentParser clap;
  clap.AddOption("help",1);
  clap.AddOption("list",0);
  clap.AddOption("test",1);
  clap.AddOption("type",1);

  // Parse the command line
  CommandLineArgumentParseResult parms;
  if(!clap.TryParseCommandLine(argc,argv,parms,false))
    {
    PrintUsage();
    return;
    }

  // Check if the user wants help
  if(parms.IsOptionPresent("help"))
    {
    // Get the file name
    const char *name = parms.GetOptionParameter("help");
    
    // Create a test instance, ingoring type
    TestBase *test = CreateTestInstance(name);

    // Print test usage
    if(test)
      {
      std::cout << "SNAPTests " << name << " options" << std::endl;
      std::cout << "Options: " << std::endl;
      test->PrintUsage();
      }
      
    else
      std::cerr << "Unknown test name: " << name << std::endl;
    }

  else if(parms.IsOptionPresent("list"))
    {
    // Print out a header
    std::cout << std::setw(20) << std::ios::left << "Test Name";
    std::cout << std::setw(12) << std::ios::left << "Templated";
    std::cout << "Description" << std::endl;
    
    // Go through the list of known tests
    for(unsigned int i=0;i<NUMBER_OF_TESTS;i++)
      {
      TestBase *test = CreateTestInstance(m_TestNames[i]);

      if(test) 
        {
        // Print out test info
        std::cout << std::setw(20) << std::ios::left << m_TestNames[i];
        std::cout << std::setw(12) << std::ios::left << (m_TestTemplated[i] ? "Yes" : "No");
        std::cout << test->GetDescription() << std::endl;
        }
      }
    }
  else if(parms.IsOptionPresent("test"))
    {
    // Get the file name
    const char *name = parms.GetOptionParameter("test");
  
    // Check if the test can be created without a template parameter
    TestBase *test = CreateNonTemplateTest(name);

    // If that failed, check if the test can be created using a template 
    // parameter
    if(!test && (test = TemplatedTestCreator<unsigned char>(name).GetTest()))
      {
      // Get the template type or a blank string
      string type = parms.IsOptionPresent("type") ? 
        parms.GetOptionParameter("type") : "";

      // Instantiate the template test of the right type
      if(type == "char")      
        test = TemplatedTestCreator<char>(name).GetTest();
      else if(type == "unsigned_char") 
        test = TemplatedTestCreator<unsigned char>(name).GetTest();
      if(type == "short")      
        test = TemplatedTestCreator<short>(name).GetTest();
      else if(type == "unsigned_short") 
        test = TemplatedTestCreator<unsigned short>(name).GetTest();
      if(type == "int")      
        test = TemplatedTestCreator<int>(name).GetTest();
      else if(type == "unsigned_int") 
        test = TemplatedTestCreator<unsigned int>(name).GetTest();
      if(type == "long")      
        test = TemplatedTestCreator<long>(name).GetTest();
      else if(type == "unsigned_long") 
        test = TemplatedTestCreator<unsigned long>(name).GetTest();
      if(type == "float")      
        test = TemplatedTestCreator<float>(name).GetTest();
      else if(type == "double") 
        test = TemplatedTestCreator<double>(name).GetTest();
      else
        {
        std::cerr << "Missing or invalid or missing type parameter.  Using default (char)" << std::endl;
        std::cerr << "Should be one of: " << std::endl;
        std::cerr << "  char, unsigned_char," << std::endl;
        std::cerr << "  short, unsigned_short," << std::endl;
        std::cerr << "  int, unsigned_int," << std::endl;
        std::cerr << "  long, unsigned_long," << std::endl;
        std::cerr << "  float, " << std::endl;
        std::cerr << "  double." << std::endl;
        }
      }

    // See if a test has been created after all
    if(test) 
      {
      try 
        {
        // Configure the parameters of the test
        test->ConfigureCommandLineParser(clap);

        // Get the command line result
        CommandLineArgumentParseResult testParms;
        if(clap.TryParseCommandLine(argc,argv,testParms,false))
          {
          test->SetCommandLineParameters(testParms);
          test->Run();
          }
        else
          {
          test->PrintUsage();
          }
        
        delete test;
        }
      catch(TestUsageException)
        {
        test->PrintUsage();
        }
      catch(itk::ExceptionObject &exc)
        {
        std::cerr << "ITK Exception: " << std::endl << exc << std::endl;
        }
      catch(...)
        {
        std::cerr << "Unknowm Exception!" << std::endl;
        }
      }
    else
      {
      std::cerr << "Could not create test!" << std::endl;
      PrintUsage();
      }    
    }
  else
    {
    PrintUsage();
    }
}
