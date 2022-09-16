/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
  
  This file was created as part of the added functionality of the custom windowing.
  We have added the option to set a custom contrast window preset using Contrast Menu.

  This file is mostly based on Common\Registry.cxx
  
=========================================================================*/
#include "ContrastXML.h"
#include "itkXMLFile.h"

#include "itksys/SystemTools.hxx"
#include "IRISException.h"

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif
#include <Registry.cxx>

using namespace std;


int ContrastXMLFileReader::CanReadFile(const char* name)
{
    return GenericXMLFileReader::CanReadFile(name);
}

std::vector<std::shared_ptr<PresetStruct>> ContrastXMLFileReader::getPresetStack() const
{
  return m_PresetStack;
}

void ContrastXMLFileReader::StartElement(const char* name, const char** atts)
{
    // If this is the root-level element <contrast>, it can be ignored
    if (strcmpi(name, "contrast") == 0)
    {
        return;
    }

    // Read the attributes into a map
    typedef std::map<std::string, std::string> AttributeMap;
    typedef AttributeMap::const_iterator AttrIter;
    AttributeMap attrmap;

    for (int i = 0; atts[i] != nullptr && atts[i + 1] != nullptr; i += 2)
        attrmap[itksys::SystemTools::LowerCase(atts[i])] = atts[i + 1];

    // Process tags
    if (strcmpi(name, "preset") == 0)
    {
        const AttrIter itKey = attrmap.find("key");
        if (itKey == attrmap.end())
            throw IRISException("Missing 'key' attribute to <preset> element");

        const auto preset_name = itKey->second;

        // Create a new preset and place it on the stack
        m_PresetStack.push_back(std::make_shared<PresetStruct>(PresetStruct{preset_name}));
    }
    else if (strcmpi(name, "contrastProperty") == 0)
    {
        const AttrIter itKey = attrmap.find("key");
        if (itKey == attrmap.end())
            throw IRISException("Missing 'key' attribute to <contrastProperty> element");

        const AttrIter itValue = attrmap.find("value");
        if (itValue == attrmap.end())
            throw IRISException("Missing 'value' attribute to <contrastProperty> element");

        const auto property_name = itKey->second;
        const auto property_value = itValue->second;

        if(property_name == "Window")
        {
            m_PresetStack.back()->window = stod(property_value);
        }
        if(property_name == "Level")
        {
          m_PresetStack.back()->level = stod(property_value);
        }
    }
    else
        throw IRISException("Unknown XML element <%s>", name);
}

void ContrastXMLFileReader::EndElement(const char* name)
{
}

void ContrastXMLFileReader::CharacterDataHandler(const char* inData, int inLength)
{
}


void
Contrast
::Clear()
{
    m_PresetMap.clear();
}

std::vector<std::string>
Contrast::GetPresetNames()
{
    // get all keys from m_PresetMap and sort if needed
    std::vector<std::string> presets;
    for (const auto& map_entry : m_PresetMap)
    {
        presets.push_back(map_entry.first); // have to get all keys
    }

    return presets;
}

double Contrast::GetLevel(const std::string& tissue)
{
    return m_PresetMap.at(tissue).level;
}

double Contrast::GetWindow(const std::string& tissue)
{
    return m_PresetMap.at(tissue).window;
}


Contrast
::Contrast()
= default;

Contrast::
~Contrast()
= default;

void Contrast::ReadFromCustomContrastXMLFile(const char* pathname, SmartPtr<ContrastXMLFileReader>& reader)
{
    reader->SetOutputObject(this);
    reader->SetFilename(pathname);

    reader->GenerateOutputInformation();
    
    // check if presets are populated
    for (const auto& preset : reader->getPresetStack())
    {
        m_PresetMap.insert(std::pair<std::string, PresetStruct>(preset->preset_name, *preset));
    }
}
