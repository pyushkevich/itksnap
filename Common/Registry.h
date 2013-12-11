/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Registry.h,v $
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
#ifndef __Registry_h_
#define __Registry_h_

#ifdef _MSC_VER
# pragma warning(disable:4786)  // '255' characters in the debug information
#endif //_MSC_VER

#include <stdio.h>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "SNAPCommon.h"
#include "itkXMLFile.h"

inline Vector2d GetValueWithDefault(const std::string &source, bool isNull, Vector2d defaultValue)
{
  // Initialize with the default value
  Vector2d returnValue = defaultValue;

  // Default value is returned if the entry is Null
  if(isNull)
    return returnValue;

  // Try to access the value using c++ formatting
  IRISIStringStream iss(source);
  iss >> returnValue;
  
  // Proceed as if the operation succeeded
  return returnValue;
}

inline Vector3d GetValueWithDefault(const std::string &source, bool isNull, Vector3d defaultValue)
{
  // Initialize with the default value
  Vector3d returnValue = defaultValue;

  // Default value is returned if the entry is Null
  if(isNull)
    return returnValue;

  // Try to access the value using c++ formatting
  IRISIStringStream iss(source);
  iss >> returnValue;
  
  // Proceed as if the operation succeeded
  return returnValue;
}

 
inline Vector3i GetValueWithDefault(const std::string &source, bool isNull, Vector3i defaultValue)
{
  // Initialize with the default value
  Vector3i returnValue = defaultValue;

  // Default value is returned if the entry is Null
  if(isNull)
    return returnValue;

  // Try to access the value using c++ formatting
  IRISIStringStream iss(source);
  iss >> returnValue;
  
  // Proceed as if the operation succeeded
  return returnValue;
}

inline Vector2i GetValueWithDefault(const std::string &source, bool isNull, Vector2i defaultValue)
{
  // Initialize with the default value
  Vector2i returnValue = defaultValue;

  // Default value is returned if the entry is Null
  if(isNull)
    return returnValue;

  // Try to access the value using c++ formatting
  IRISIStringStream iss(source);
  iss >> returnValue;
  
  // Proceed as if the operation succeeded
  return returnValue;
}

template <class T>
inline T GetValueWithDefault(const std::string &source, bool isNull, T defaultValue)
{
  // Initialize with the default value
  T returnValue = defaultValue;

  // Default value is returned if the entry is Null
  if(isNull)
    return returnValue;

  // Try to access the value using c++ formatting
  IRISIStringStream iss(source);
  iss >> returnValue;

  // Proceed as if the operation succeeded
  return returnValue;
}

template <>
inline const char *GetValueWithDefault<const char *>(const std::string &source, bool isNull, const char *defaultValue)
{
  if(isNull)
    return defaultValue;
  else
    return source.c_str();
}

/** A class used to associate a set of strings with an ENUM so that the
 * enum can be exported to the registry in a consistent fashion */
template <class TEnum>
class RegistryEnumMap
{
public:
  typedef std::string StringType;
  void AddPair(TEnum value, const char *description)
  {
    m_EnumToStringMap[value] = std::string(description);
    m_StringToEnumMap[description] = value;
  }

  bool GetEnumValue(const StringType &key, TEnum &outValue) const
  {
    typename std::map<StringType, TEnum>::const_iterator it = m_StringToEnumMap.find(key);
    if(it == m_StringToEnumMap.end())
      return false;

    outValue = it->second;
    return true;
  }

  bool GetString(TEnum value, StringType &outString) const
  {
    typename std::map<TEnum, StringType>::const_iterator it = m_EnumToStringMap.find(value);
    if(it == m_EnumToStringMap.end())
      return false;

    outString = it->second;
    return true;
  }

  unsigned int Size() const { return m_EnumToStringMap.size(); }
  StringType operator [] (TEnum value) { return m_EnumToStringMap[value]; }
private:
  std::map<TEnum, StringType> m_EnumToStringMap;
  std::map<StringType, TEnum> m_StringToEnumMap;
  friend class RegistryValue;
};

/** A class that represents a value in the registry */
class RegistryValue
{
public:
  /** Default constructor: sets this object to Null state */
  RegistryValue();

  /** Initializing constructor: sets the object to a value */
  RegistryValue(const std::string &value);

  /** Comparison */
  bool operator == (const RegistryValue &other) const
  {
    return m_Null == other.m_Null && m_String == other.m_String;
  }

  bool operator != (const RegistryValue &other) const
  {
    return ! (*this == other);
  }

  /** Is the value present in the Registry? */
  bool IsNull() const { return m_Null; }

  /** Get the internally stored string */
  const std::string &GetInternalString() const { return m_String; }

  /**
   * An operator that allows us to access a value using different
   * formats
   */
  bool operator[](bool defaultValue) {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  int operator[](int defaultValue) {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  unsigned int operator[](unsigned int defaultValue) {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  double operator[](double defaultValue) {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  const char *operator[](const char *defaultValue) {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  std::string operator[](const std::string &defaultValue) {
    if(IsNull())
      return defaultValue;
    else
      return m_String;
  }
  

  Vector3i operator[](const Vector3i &defaultValue)
  {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  Vector2i operator[](const Vector2i &defaultValue)
  {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  Vector3d operator[](const Vector3d &defaultValue)
  {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

  Vector2d operator[](const Vector2d &defaultValue)
  {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }

/*
  template <class T, int VSize> iris_vector_fixed<T,VSize>
    operator[](const iris_vector_fixed<T,VSize> &defaultValue)
  {
    return GetValueWithDefault(m_String,m_Null,defaultValue);
  }
*/

  /**
   * An operator that allows us to write a value using different formats
   */
  template <class T> void operator << (const T newValue)
  {
    // Create an output stream
    IRISOStringStream oss;

    // Put the new value into the stream
    oss << newValue;

    // Set the string
    m_String = oss.str();
    m_Null = false;
  }

  /**
   * Put an enum into this entry. If the enum map does not have the value,
   * nothing will be written to the entry (entry will be null)
   */
  template <class TEnum> void PutEnum(const RegistryEnumMap<TEnum> &rem, TEnum value)
  {
    m_Null = !rem.GetString(value, m_String);
  }

  /** Get an enum from this entry */
  template <class TEnum> TEnum GetEnum(const RegistryEnumMap<TEnum> &rem, TEnum defaultValue)
  {
    TEnum value;
    if(rem.GetEnumValue(m_String, value))
      return value;
    else return defaultValue;
  }

private:
  std::string m_String;
  bool m_Null;
};




/**
 * \class Registry
 * \brief A tree of key-value pair maps
 */
class Registry
{
public:
  // String definition
  typedef std::string StringType;

  // List of strings
  typedef std::list<StringType> StringListType;

  /** Constructor initializes an empty registry */
  Registry();

  /** Constructor loads a registry from a file */
  Registry(const char *fname);

  /** Destructor */
  virtual ~Registry();

  /** Comparison */
  bool operator == (const Registry &other) const;

  bool operator != (const Registry &other) const;

  /** Get a reference to a value in this registry, which can then be queried */
  RegistryValue &operator[](const StringType &key) { return Entry(key); }

  /** Get a reference to a folder inside this registry, creating it if necessary */
  RegistryValue &Entry(const StringType &key);

  /** Get a reference to a folder inside this registry, creating it if necessary */
  Registry &Folder(const StringType &key);

  /** A helper method to convert a printf-style expression to a key */
  static StringType Key(const char *key,...);

  /** Get a list of all keys that have values, append it to keyList */
  int GetEntryKeys(StringListType &keyList);

  /** Get a list of all subfolder keys, append it to keyList */
  int GetFolderKeys(StringListType &keyList);

  /** Check if an entry with the given key exists */
  bool HasEntry(const StringType &key);

  /** Check if a subfolder with the given key exists */
  bool HasFolder(const StringType &key);

  /** Find a value in a folder or return "" */
  StringType FindValue(const StringType& value);

  /** Empty the contents of the registry */
  void Clear();

  /** Get a list of all keys that have values contained in this registry
   * and all subfolders (recursive operation).  Prefix is used internally,
   * but can be specified to prepend a string to all keys */
  void CollectKeys(StringListType &keyList,const StringType &keyPrefix="");

  /** Remove all keys from this folder that start with a string */
  void RemoveKeys(const char *match = NULL);

  /** Write this Registry to an file.  The second parameter is an optional
   * header, each line of which must start with the '#" character  */
  void WriteToFile(const char *pathname, const char *header = NULL);

  /** Write the Registry to an XML file */
  void WriteToXMLFile(const char *pathname, const char *header = NULL);

  /**
   * Update the registry with keys from another registry
   */
  void Update(const Registry &reg);

  /** Read this Registry from a file */
  void ReadFromFile(const char *pathname);

  /** Read from an std::ifstream */
  void ReadFromStream(std::istream &sin);

  /** Read from XML file */
  void ReadFromXMLFile(const char *pathname);


  /** Experimental */
  void SetFlagAddIfNotFound(bool yesno);

  /** Put an array into the registry */
  template <class T> void PutArray(unsigned int size,const T *array)
  {
    RemoveKeys("Element");
    Entry("ArraySize") << size;
    for(unsigned int i=0;i<size;i++)
      {
      Entry(Key("Element[%d]",i)) << array[i];
      }
  }

  /** Put an array into the registry */
  template <class T> void PutArray(const std::vector<T> &array)
  {
    RemoveKeys("Element");
    Entry("ArraySize") << array.size();
    for(unsigned int i=0;i<array.size();i++)
      {
      Entry(Key("Element[%d]",i)) << array[i];
      }
  }

  /** Get an array from the registry */
  template <class T> std::vector<T> GetArray(const T &defaultElement)
  {
    // Try reading the element count
    unsigned int size = Entry("ArraySize")[(unsigned int) 0];

    // Initialize the result array
    std::vector<T> result(size,defaultElement);

    // Read element
    for(unsigned int i=0;i < size;i++)
      {
      result[i] = Entry(Key("Element[%d]",i))[defaultElement];
      }

    // Return the array
    return result;
  }

  /** An IO exception objects thrown by this class when reading from file*/
  class IOException : public StringType {
  public:
    IOException(const char *text) : StringType(text) {}
  };

  /** A syntax error exception */
  class SyntaxException : public StringType {
  public:
    SyntaxException(const char *text) : StringType(text) {}
  };

private:
  // Hashtable type definition
  typedef std::map<StringType, Registry *> FolderMapType;
  typedef std::map<StringType, RegistryValue> EntryMapType;

  // Commonly used hashtable iterators
  typedef FolderMapType::const_iterator FolderIterator;
  typedef EntryMapType::iterator EntryIterator;
  typedef EntryMapType::const_iterator EntryConstIterator;

  /** A hash table for the subfolders */
  FolderMapType m_FolderMap;

  /** A hash table for the registry values */
  EntryMapType m_EntryMap;

  /**
   * A flag as to whether keys and folders that are read and not found
   * should be created and populated with default values.
   */
  bool m_AddIfNotFound;

  /** Write this folder recursively to a stream */
  void Write(std::ostream &sout,const StringType &keyPrefix);

  /** Write this folder recursively to a stream in XML format */
  void WriteXML(std::ostream &sout,const StringType &keyPrefix);

  /** Read this folder recursively from a stream, recording syntax errors */
  void Read(std::istream &sin, std::ostream &serr);

  /** Encode a string for writing to file */
  static StringType Encode(const StringType &input);

  /** Decode a string for writing to file */
  static StringType Decode(const StringType &input);

  /** Encode a string for writing to file */
  static StringType EncodeXML(const StringType &input);

  /** Decode a string for writing to file */
  static StringType DecodeXML(const StringType &input);
};


#include <itkXMLFile.h>


#endif
