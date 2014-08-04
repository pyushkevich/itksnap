/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Registry.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
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
#include "Registry.h"
#include "IRISVectorTypes.h"
#include "itkXMLFile.h"

#include <stdio.h>
#include <cstdlib>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include "itksys/SystemTools.hxx"
#include "IRISException.h"

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

using namespace std;



/** Reader for XML files */
class RegistryXMLFileReader : public itk::XMLReader<Registry>
{
public:

  irisITKObjectMacro(RegistryXMLFileReader, itk::XMLReader<Registry>)

protected:

  RegistryXMLFileReader() {}

  virtual int CanReadFile(const char *name);
  virtual void StartElement(const char *name, const char **atts);
  virtual void EndElement(const char *name);
  virtual void CharacterDataHandler(const char *inData, int inLength);

  static int strcmpi(const char *str1, const char *str2)
  {
    return itksys::SystemTools::Strucmp(str1, str2);
  }

  // The folder stack
  std::list<Registry *> m_FolderStack;
};

int RegistryXMLFileReader::CanReadFile(const char *name)
{
  if ( !itksys::SystemTools::FileExists(name)
       || itksys::SystemTools::FileIsDirectory(name)
       || itksys::SystemTools::FileLength(name) == 0 )
    {
    return 0;
    }

  return 1;
}

void RegistryXMLFileReader::StartElement(const char *name, const char **atts)
{
  // If this is the root-level element <registry>, it can be ignored
  if(strcmpi(name, "registry") == 0)
    {
    // Put the target registry on the stack
    assert(this->GetOutputObject());
    m_FolderStack.push_back(this->GetOutputObject());
    return;
    }

  // If the stack is empty throw up
  if(m_FolderStack.size() == 0)
    throw IRISException("Problem parsing Registry XML file. The file might not be valid.");

  // Read the attributes into a map
  typedef std::map<std::string, std::string> AttributeMap;
  typedef AttributeMap::const_iterator AttrIter;
  AttributeMap attrmap;

  for(int i = 0; atts[i] != NULL && atts[i+1] != NULL; i+=2)
    attrmap[itksys::SystemTools::LowerCase(atts[i])] = atts[i+1];

  // Process tags
  if(strcmpi(name, "folder") == 0)
    {
    AttrIter itKey = attrmap.find("key");
    if(itKey == attrmap.end())
      throw IRISException("Missing 'key' attribute to <folder> element");

    // Create a new folder and place it on the stack
    Registry &newFolder = m_FolderStack.back()->Folder(itKey->second);
    m_FolderStack.push_back(&newFolder);
    }
  else if(strcmpi(name, "entry") == 0)
    {
    AttrIter itKey = attrmap.find("key");
    if(itKey == attrmap.end())
      throw IRISException("Missing 'key' attribute to <entry> element");

    AttrIter itValue = attrmap.find("value");
    if(itValue == attrmap.end())
      throw IRISException("Missing 'value' attribute to <entry> element");

    // Create a new entry (TODO: decode!)
    m_FolderStack.back()->Entry(itKey->second) = RegistryValue(itValue->second);
    }
  else
    throw IRISException("Unknown XML element <%s>", name);
}


void RegistryXMLFileReader::EndElement(const char *name)
{
  if(strcmpi(name, "registry") == 0)
    {
    m_FolderStack.clear();
    }

  else if(strcmpi(name, "folder") == 0)
    {
    m_FolderStack.pop_back();
    }
}

void RegistryXMLFileReader::CharacterDataHandler(const char *inData, int inLength)
{
  return;
}


RegistryValue
::RegistryValue()
{
  m_Null = true;
  m_String = "";  
}

RegistryValue
::RegistryValue(const std::string &input)
{
  m_Null = false;
  m_String = input;  
}

RegistryValue&
Registry::Entry(const std::string &key)
{
  // Get the containing folder
  StringType::size_type iDot = key.find_first_of('.');

  // There is a subfolder
  if(iDot != key.npos)
    {
    StringType child = key.substr(0,iDot);
    StringType childKey = key.substr(iDot+1);
    return Folder(child).Entry(childKey);
    }

  // Search for the key and return it if found
  EntryIterator it = m_EntryMap.find(key);
  if(it != m_EntryMap.end())
    return it->second;

  // Key was not found, create a null entry
  m_EntryMap[key] = RegistryValue();
  return m_EntryMap[key];
}

Registry::StringType
Registry::Key(const char *format,...)
{
  // A string for prinf-ing
  static char buffer[1024];
  
  // Do the printf operation
  va_list al;
  va_start(al,format);
  vsprintf(buffer,format,al);
  va_end(al);

  // Use the string as parameter
  return StringType(buffer);  
}

int
Registry
::GetEntryKeys(StringListType &targetArray) 
{
  // Iterate through keys in ascending order
  for(EntryIterator it=m_EntryMap.begin();it!=m_EntryMap.end();++it)
    {
    // Put the key in the array
    targetArray.push_back(it->first);
    }

  // Return the number of keys copied
  return targetArray.size();
}

int
Registry
::GetFolderKeys(StringListType &targetArray) 
{
  // Iterate through keys in ascending order
  for(FolderIterator it=m_FolderMap.begin();it!=m_FolderMap.end();++it)
    {
    // Put the key in the array
    targetArray.push_back(it->first);
    }

  // Return the number of keys copied
  return targetArray.size();
}

bool Registry::HasEntry(const Registry::StringType &key)
{
  // Get the containing folder
  StringType::size_type iDot = key.find_first_of('.');

  // There is a subfolder
  if(iDot != key.npos)
    {
    StringType child = key.substr(0,iDot);
    StringType childKey = key.substr(iDot+1);
    return Folder(child).HasEntry(childKey);
    }

  // Search for the key and return it if found
  EntryIterator it = m_EntryMap.find(key);
  return it != m_EntryMap.end();
}

bool Registry::HasFolder(const Registry::StringType &key)
{
  // Get the containing folder
  StringType::size_type iDot = key.find_first_of('.');

  // There is a subfolder
  if(iDot != key.npos)
    {
    StringType child = key.substr(0,iDot);
    StringType childKey = key.substr(iDot+1);
    return Folder(child).HasFolder(childKey);
    }

  // Search for the key and return it if found
  FolderIterator it = m_FolderMap.find(key);
  return it != m_FolderMap.end();
}

void
Registry
::Write(ostream &sout,const StringType &prefix)
{
  // Write the entries in this folder
  for(EntryIterator ite = m_EntryMap.begin();ite != m_EntryMap.end(); ++ite)
    {
    // Only write the non-null entries
    if(!ite->second.IsNull())
      {
      // Write the key = 
      sout << prefix << Encode(ite->first) << " = ";

      // Write the encoded value
      sout << Encode(ite->second.GetInternalString()) << endl;
      }
    }

  // Write the folders
  for(FolderIterator itf = m_FolderMap.begin(); itf != m_FolderMap.end(); ++itf)
    {
    // Write the folder contents (recursive, contents prefixed with full path name)
    itf->second->Write(sout, prefix + itf->first + "." );
    }  
}

void
Registry
::WriteXML(ostream &sout, const StringType &prefix)
{
  // Write the entries in this folder
  for(EntryIterator ite = m_EntryMap.begin();ite != m_EntryMap.end(); ++ite)
    {
    // Only write the non-null entries
    if(!ite->second.IsNull())
      {
      // Write the key
      sout << prefix << "<entry key=\"" << EncodeXML(ite->first) << "\"";

      // Write the encoded value
      sout << " value=\"" << EncodeXML(ite->second.GetInternalString()) << "\" />" << endl;
      }
    }

  // Write the folders
  for(FolderIterator itf = m_FolderMap.begin(); itf != m_FolderMap.end(); ++itf)
    {
    // Write the folder tag
    sout << prefix << "<folder key=\"" << EncodeXML(itf->first) << "\" >" << endl;

    // Write the folder contents (recursive, contents prefixed with full path name)
    itf->second->WriteXML(sout, prefix + "  ");

    // Close the folder
    sout << prefix << "</folder>" << endl;
    }
}

void 
Registry
::SetFlagAddIfNotFound(bool yesno) 
{
  // Set the internal value
    m_AddIfNotFound = yesno;

  // Propagate to all the children folders
  for(FolderIterator itf = m_FolderMap.begin(); itf != m_FolderMap.end(); ++itf)
    {
    itf->second->SetFlagAddIfNotFound(yesno);
    }
}

void 
Registry
::CollectKeys(StringListType &keyList,const StringType &prefix) 
{
  // Go through the children
  for(FolderIterator itf = m_FolderMap.begin(); itf != m_FolderMap.end(); ++itf)
    {
    // Collect the child's keys with a new prefix
    itf->second->CollectKeys(keyList, prefix + itf->first + ".");
    }
  
  // Add the keys in this folder
  for(EntryIterator ite = m_EntryMap.begin();ite != m_EntryMap.end(); ++ite)
    {
    // Add the key to the collection list
    keyList.push_back(prefix + ite->first);
    }
}

void 
Registry
::Update(const Registry &reg) 
{
  // Go through the children
  for(FolderIterator itf = reg.m_FolderMap.begin(); 
    itf != reg.m_FolderMap.end(); ++itf)
    {
    // Update the sub-folder
    this->Folder(itf->first).Update(*(itf->second));
    }
  
  // Add the keys in this folder
  for(EntryConstIterator ite = reg.m_EntryMap.begin();
    ite != reg.m_EntryMap.end(); ++ite)
    {
    RegistryValue &entry = Entry(ite->first);
    entry = ite->second;   
    }
}




Registry::StringType 
Registry
::FindValue(const StringType& value)
{
  // Add the keys in this folder
  for(EntryIterator ite = m_EntryMap.begin();ite != m_EntryMap.end(); ++ite)
    {
    if(ite->second.GetInternalString() == value)
      return ite->first;
    }
  return "";
}

void 
Registry
::RemoveKeys(const char *match)
{
  // Create a match substring
  string sMatch = (match) ? match : 0;

  // Search and delete from the map
  EntryMapType newMap;
  for(EntryIterator it=m_EntryMap.begin(); it != m_EntryMap.end(); it++)
    {
    if(it->first.find(sMatch) != 0)
      newMap[it->first] = it->second;
    }

  m_EntryMap = newMap;
}

void
Registry
::Clear()
{
  m_EntryMap.clear();
  m_FolderMap.clear();
}

Registry::StringType
Registry
::EncodeXML(const StringType &input)
{
  IRISOStringStream oss;
  for(unsigned int i=0; i < input.length() ; i++)
    {
    // Map the character to positive integer (0..255)
    char c = input[i];

    // There are special characters not allowed in XML
    switch(c)
      {
      case '<' :
        oss << "&lt;"; break;
      case '>' :
        oss << "&gt;"; break;
      case '&' :
        oss << "&amp;"; break;
      case '\'' :
        oss << "&apos;"; break;
      case '\"' :
        oss << "&quot;"; break;
      default:
        oss << c; break;
      }
   }

  // Return the resulting string
  return oss.str();
}

Registry::StringType Registry::DecodeXML(const Registry::StringType &input)
{
  return input;
}

Registry::StringType
Registry
::Encode(const StringType &input) 
{
  IRISOStringStream oss;
  for(unsigned int i=0; i < input.length() ; i++)
    {
    // Map the character to positive integer (0..255)
    char c = input[i];
    int v = static_cast<int>(static_cast<unsigned char>(c));

    // We only print ASCII codes
    if(v <= 0x20 || v >= 0x7f || c == '%')
      {
      // Replace character by a escape string
      oss << '%';
      oss << setw(2) << setfill('0') << hex << v;
      }
    else
      {
      // Just copy the character
      oss << c;
      }
    }

  // Return the resulting string
  return oss.str();
}   

Registry::StringType
Registry::Decode(const StringType &input) 
{
  // Create an input stream
  IRISIStringStream iss(input);
  IRISOStringStream oss;

  // Read until we run out or crash
  while(iss.good())
    {  
    // Read the next character
    char c = (char) iss.get();

    // Check if the character needs to be translated
    if(!isprint(c))
      {
      continue;
      }
    else if(c != '%')
      {
      // Just copy the character
      oss.put(c);
      }
    else 
      {
      // A pair of chars
      char c1,c2;
      
      // Read the hex digit
      iss >> c1;
      iss >> c2;
      
      // Reconstruct the hex
      int d1 = (c1 < 'a') ? c1 - '0' : c1 - 'a' + 10;
      int d2 = (c2 < 'a') ? c2 - '0' : c2 - 'a' + 10;
      int digit = d1 * 16 + d2;

      // See if the read succeeded, only then use the digit
      if(iss.good())
        oss << (char)digit;      
      
      // A good place to throw an exception (for strict interpretation)
      }        
    }
  
  // Return the result
  return oss.str();
}


void
Registry
::Read(istream &sin, ostream &oss) 
{
  unsigned int lineNumber = 1;
  while(sin.good())
    {
    // Read a line from the file
    char buffer[1024];
    sin.getline(buffer,1024);

    // Find the first character in the string
    StringType line = buffer;
    StringType::size_type iToken = line.find_first_not_of(" \t\v\r\n");

    // Skip blank lines
    if(iToken == line.npos)
      continue;

    // Check if it's a comment
    if(line[iToken] == '#')
      continue;

    // Find an equal sign in the string
    StringType::size_type iOper = line.find_first_of('=');
    if(iOper == line.npos)
      {
      // Not a valid line
      oss << std::setw(5) << lineNumber << " : Missing '='; line ignored." << endl;
      continue;
      }
    if(iOper == iToken)
      {
      // Missing key
      oss << std::setw(5) << lineNumber << " : Missing key before '='; line ignored." << endl;
      continue;
      }

    // Extract the key
    string key = Decode(
      line.substr(iToken,line.find_first_of(" \t\v\r\n=",iToken) - iToken));

    // Extract the value
    StringType::size_type iValue = line.find_first_not_of(" \t\v\r\n=",iOper);
    string value = "";
    if (iValue != line.npos) 
      {
      value = line.substr(iValue);
      }
 
    // Now the key-value pair is present.  Add it using the normal interface
    Entry(key) = RegistryValue(Decode(value));

    ++lineNumber;
    }
}

Registry &
Registry
::Folder(const string &key) 
{
  // Find the first separator in the key string
  StringType::size_type iDot = key.find_first_of('.');

  // Recurse on the immideate folder
  if(iDot != key.npos) 
    {
    StringType child = key.substr(0,iDot);
    StringType childKey = key.substr(iDot+1);
    return Folder(child).Folder(childKey);
    }

  // Get the folder, adding if necessary
  FolderIterator it = m_FolderMap.find(key);
  if(it != m_FolderMap.end())
    return *(it->second);

  // Add the folder
  m_FolderMap[key] = new Registry();
  m_FolderMap[key]->m_AddIfNotFound = m_AddIfNotFound;
  return *m_FolderMap[key];
}

Registry
::Registry()
{
  m_AddIfNotFound = false;
}

Registry
::Registry(const char *fname) 
{
  ReadFromFile(fname);
}

Registry::
~Registry() 
{
  // Delete all the sub-folders
  // for(FolderIterator itf = m_FolderMap.begin(); itf != m_FolderMap.end(); ++itf)
  //  delete itf->second;
}

bool Registry::operator == (const Registry &other) const
{
  // Compare the folders
  if(m_FolderMap.size() != other.m_FolderMap.size())
    return false;

  for(FolderIterator it1 = m_FolderMap.begin(), it2 = other.m_FolderMap.begin();
      it1 != m_FolderMap.end(); ++it1, ++it2)
    {
    // Compare keys
    if(it1->first != it2->first)
      return false;

    // Compare subfolder contents (recursively)
    if(*(it1->second) != *(it2->second))
      return false;
    }

  // Compare the entries
  if(m_EntryMap.size() != other.m_EntryMap.size())
    return false;

  for(EntryConstIterator it1 = m_EntryMap.begin(), it2 = other.m_EntryMap.begin();
      it1 != m_EntryMap.end(); ++it1, ++it2)
    {
    // Compare keys
    if(it1->first != it2->first)
      return false;

    // Compare subfolder contents (recursively)
    if(it1->second != it2->second)
      return false;
    }

  return true;
}

bool Registry::operator != (const Registry &other) const
{
  return ! (*this == other);
}


/**
* Write the folder to a disk
*/
void 
Registry
::WriteToFile(const char *pathname, const char *header) 
{
  // Open the file
  ofstream sout(pathname,std::ios::out);

  // Set the stream to be picky
  sout.exceptions(std::ios::failbit);

  // Write the header
  if(header)
    sout << header << endl;
 
  // Write to the stream
  Write(sout,"");
}

void Registry::WriteToXMLFile(const char *pathname, const char *header)
{
  // Open the file
  ofstream sout(pathname,std::ios::out);

  // Set the stream to be picky
  sout.exceptions(std::ios::failbit);

  // Write the XML string
  sout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;

  // Write the header
  if(header)
    sout << "<!--" << header << "-->" << endl;

  // Write the DOCTYPE content
  sout << "<!DOCTYPE registry [" << endl
       << "<!ELEMENT registry (entry*,folder*)>" << endl
       << "<!ELEMENT folder (entry*,folder*)>" << endl
       << "<!ELEMENT entry EMPTY>" << endl
       << "<!ATTLIST folder key CDATA #REQUIRED>" << endl
       << "<!ATTLIST entry key CDATA #REQUIRED>" << endl
       << "<!ATTLIST entry value CDATA #REQUIRED>" << endl
       << "]>" << endl;

  // Write to the stream
  sout << "<registry>" << endl;
  WriteXML(sout, "  ");
  sout << "</registry>" << endl;
}




/**
* Read folder from file
*/
void 
Registry
::ReadFromFile(const char *pathname) 
{
  // Create output stream
  ifstream sin(pathname,std::ios::in);
  if(!sin.good())
    throw IOException("Unable to open the Registry file");

  ReadFromStream(sin);

}

void Registry
::ReadFromStream(istream &sin)
{
  // Create an error stream
  IRISOStringStream serr;

  try
    {
    // Try reading
    Read(sin,serr);
    }
  catch(...)
    {
    throw IOException("Unable to read the Registry file");
    }

  // If the error stream is not empty, throw an exception
  if(serr.str().length())
    throw SyntaxException(serr.str().c_str());
}

void Registry::ReadFromXMLFile(const char *pathname)
{
  SmartPtr<RegistryXMLFileReader> reader = RegistryXMLFileReader::New();
  reader->SetOutputObject(this);
  reader->SetFilename(pathname);
  reader->GenerateOutputInformation();
}


