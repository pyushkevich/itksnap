/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: CommandLineArgumentParser.h,v $
  Language:  C++
  Date:      $Date: 2008/11/20 05:10:39 $
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
#ifndef __CommandLineArgumentParser_h_
#define __CommandLineArgumentParser_h_

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif

#include <vector>
#include <string>
#include <list>
#include <map>

/**
 * \class CommandLineArgumentParseResult
 * \brief Object returned by CommandLineArgumentParser
 * \see CommandLineArgumentParser
 */
class CommandLineArgumentParseResult
{
public:
  /** Check whether the option was passed in or not */
  bool IsOptionPresent(const char *option);

  /** Get one of the parameters to the option */
  const char *GetOptionParameter(const char *option, unsigned int number = 0);

  /** Get the number of parameters for the option */
  int GetNumberOfOptionParameters(const char *option);

private:
  typedef std::vector< std::string > ParameterArrayType;
  typedef std::map< std::string, ParameterArrayType> OptionMapType;

  void Clear();

  /**
   * @brief Add an option with specified number of parameters. The number of
   * parameters may be negative, in which case all non-options trailing the
   * command are read as parameters.
   * @param option
   * @param nParms
   */
  void AddOption(const std::string &option, int nParms);
  void AddParameter(const std::string &option, const std::string &parameter);

  OptionMapType m_OptionMap;

  friend class CommandLineArgumentParser;
};

/**
 * \class CommandLineArgumentParser
 * \brief Used to parse command line arguments and come back with a list
 * of parameters.
 * Usage:
 * \code
 *    // Set up the parser
 *    CommandLineArgumentParser parser;
 *    parser.AddOption("-f",1);
 *    parser.AddSynonim("-f","--filename");
 *    parser.AddOption("-v",0);
 *    parser.AddSynonim("-v","--verbose");
 *
 *    // Use the parser
 *    CommandLineArgumentParseResult result;
 *    if(parser.TryParseCommandLine(argc,argv,result)) {
 *       if(result.IsOptionPresent("-f"))
 *          cout << "Filename " << result.GetOptionParameter("-f") << endl;
 *       ...
 *    } 
 * \endcode      
 */
class CommandLineArgumentParser
{
public:
  /** Add an option with 0 or more parameters (words that follow it) */
  void AddOption(const char *name, int nParameters);
  
  /** Add a different string that envokes the same option (--file and -f) */  
  void AddSynonim(const char *option, const char *synonim);

  /** Try processing a command line.  Returns false if something breaks */
  bool TryParseCommandLine(int argc, char *argv[], 
                           CommandLineArgumentParseResult &outResult,
                           bool failOnUnknownTrailingParameters)
    {
    int junk;
    return this->TryParseCommandLine(argc, argv, outResult, failOnUnknownTrailingParameters, junk);
    }

  /** This version returns the index to the first unparsed parameter */
  bool TryParseCommandLine(int argc, char *argv[], 
                           CommandLineArgumentParseResult &outResult,
                           bool failOnUnknownTrailingParameters,
                           int &argc_out);
private:
private:
  // Synonim list type
  typedef std::list< std::string > NameListType;
  typedef struct 
    {
    std::string CommonName;
    unsigned int NumberOfParameters;
    } OptionType;
  typedef std::map< std::string, OptionType> OptionMapType;

  OptionMapType m_OptionMap;
};

#endif // __CommandLineArgumentParser_h_
