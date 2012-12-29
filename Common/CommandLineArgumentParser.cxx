/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: CommandLineArgumentParser.cxx,v $
  Language:  C++
  Date:      $Date: 2008/12/01 21:27:25 $
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
#include "CommandLineArgumentParser.h"

#include <assert.h>
#include <iostream>

using namespace std;

void 
CommandLineArgumentParser
::AddOption(const char *name, int nParameters)
{
  // Create a structure for the command
  OptionType option;
  option.CommonName = string(name);
  option.NumberOfParameters = nParameters;

  // Add the option to the map
  m_OptionMap[string(name)] = option;
}

void 
CommandLineArgumentParser
::AddSynonim(const char *option, const char *synonim)
{
  string strOption(option);
  string strSynonim(synonim);

  // The option should exist!
  assert(m_OptionMap.find(strOption) != m_OptionMap.end());

  // Create a new option object
  OptionType o;
  o.NumberOfParameters = m_OptionMap[strOption].NumberOfParameters;
  o.CommonName = strOption;

  // Insert the option into the map
  m_OptionMap[strSynonim] = o;
}

bool 
CommandLineArgumentParser
::TryParseCommandLine(int argc, char *argv[], 
                      CommandLineArgumentParseResult &outResult,
                      bool failOnUnknownTrailingParameters, int &argc_out)
{
  // Clear the result
  outResult.Clear();

  // Go through the arguments
  for(argc_out=1; argc_out < argc; argc_out++)
    {
    // Get the next argument
    string arg(argv[argc_out]);

    // Check if the argument is known
    if(m_OptionMap.find(arg) == m_OptionMap.end())
      {
      if(failOnUnknownTrailingParameters)
        {
        // Unknown argument found
        cerr << "Unrecognized command line option '" << arg << "'" << endl;
        return false;
        }
      else if(arg[0] == '-')
        {
        cerr << "Ignoring unknown command line option '" << arg << "'" << endl;
        continue;
        }
      else
        {
          return true;
        }
      }

    // Check if the number of parameters is correct
    int nParameters = m_OptionMap[arg].NumberOfParameters;
    if(nParameters > 0 && argc_out+nParameters >= argc)
      {
      // Too few parameters
      cerr << "Too few parameters to command line option '" << arg 
        << "'" << endl;
      return false;
      }

    // If the number of parameters is negative, read all parameters that are
    // not recognized options
    if(nParameters < 0)
      {
      nParameters = 0;
      for(int j = argc_out+1; j < argc; j++, nParameters++)
        if(m_OptionMap.find(argv[j]) != m_OptionMap.end())
          break;
      }

    // Tell the result that the option has been encountered
    outResult.AddOption(m_OptionMap[arg].CommonName,nParameters);

    // Pass in the parameters
    for(int j=0;j<nParameters;j++,argc_out++)
      outResult.AddParameter(m_OptionMap[arg].CommonName,string(argv[argc_out+1]));
    }

  // Everything is good
  return true;
}



bool 
CommandLineArgumentParseResult
::IsOptionPresent(const char *option)
{
  return (m_OptionMap.find(string(option)) != m_OptionMap.end());
}

const char * 
CommandLineArgumentParseResult
::GetOptionParameter(const char *option, unsigned int number)
{
  assert(IsOptionPresent(option));
  assert(number < m_OptionMap[string(option)].size());

  return m_OptionMap[string(option)][number].c_str();
}

int
CommandLineArgumentParseResult
::GetNumberOfOptionParameters(const char *option)
{
  assert(IsOptionPresent(option));
  return m_OptionMap[string(option)].size();
}

void  
CommandLineArgumentParseResult
::Clear()
{
  m_OptionMap.clear();
}

void  
CommandLineArgumentParseResult
::AddOption(const std::string &option, int nParms)
{
  ParameterArrayType pat;
  pat.reserve(nParms);
  m_OptionMap[option] = pat;
}

void  
CommandLineArgumentParseResult
::AddParameter(const std::string &option, const std::string &parameter)
{
  m_OptionMap[option].push_back(parameter);  
}

