/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
  
  This file was created as part of the added functionality of the custom windowing.
  We have added the option to set a custom contrast window preset using Contrast Menu.

  This file is mostly based on Common\Registry.h
  
=========================================================================*/
#ifndef __Contrast_h_
#define __Contrast_h_

#ifdef _MSC_VER
# pragma warning(disable:4786)  // '255' characters in the debug information
#endif //_MSC_VER

#include <list>
#include <map>
#include <string>
#include <cstdarg>
#include <iomanip>

#include <itksys/SystemTools.hxx>
#include "itkXMLFile.h"

#include "IRISException.h"

struct PresetStruct
{
  std::string preset_name;
  double level;
  double window;
};

class Contrast;

/** Reader for XML files */
class ContrastXMLFileReader : public itk::XMLReader<Contrast>
{
public:

    irisITKObjectMacro(ContrastXMLFileReader, itk::XMLReader<Contrast>)
    int CanReadFile(const char* name) ITK_OVERRIDE;

    std::vector<std::shared_ptr<PresetStruct>> getPresetStack() const;

protected:

    ContrastXMLFileReader()
    = default;

    void StartElement(const char* name, const char** atts) ITK_OVERRIDE;
    void EndElement(const char* name) ITK_OVERRIDE;
    void CharacterDataHandler(const char* inData, int inLength) ITK_OVERRIDE;

    static int strcmpi(const char* str1, const char* str2)
    {
        return itksys::SystemTools::Strucmp(str1, str2);
    }
    
    // The preset stack
    std::vector<std::shared_ptr<PresetStruct>> m_PresetStack;
};

/**
 * \class Contrast
 * \brief A tree of key-value pair maps
 */
class Contrast final
{
public:
    /** Constructor initializes an empty contrast */
    Contrast();

    /** Destructor */
    virtual ~Contrast();

    /** Get a list of preset names*/
    std::vector<std::string> GetPresetNames();

    /** Get a level value*/
    double GetLevel(const std::string& tissue);

    /** Get a window value*/
    double GetWindow(const std::string& tissue);

    /** Empty the contents of the contrast */
    void Clear();

    /** Read from XML file */
    void ReadFromCustomContrastXMLFile(const char* pathname, SmartPtr<ContrastXMLFileReader>& reader);

private:
    
    /** A hash table for the presets */
    std::map<std::string, PresetStruct> m_PresetMap;
};

#endif
