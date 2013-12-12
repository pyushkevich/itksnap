#include "ColorLabelTable.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace std;

int parse_color(
  const char* p, unsigned char& r, unsigned char& g, unsigned char& b)
{
  // Must be a seven-character string
  if(strlen(p) != 7)
    return -1;

  int val[6];
  for(int i = 0; i < 6; i++)
    {
    char c = p[i+1];
    if(c >= 'A' && c <= 'F')
      val[i] = 10 + (c - 'A');
    else if(c >= 'a' && c <= 'f')
      val[i] = 10 + (c - 'a');
    else if(c >= '0' && c <= '9')
      val[i] = c - '0';
    else return -1;
    }

  r = 16 * val[0] + val[1];
  g = 16 * val[2] + val[3];
  b = 16 * val[4] + val[5];
  return 0;
}


// Some randomly ordered colors
const size_t ColorLabelTable::m_ColorListSize = 130;
const char *ColorLabelTable::m_ColorList[ColorLabelTable::m_ColorListSize] = {
  "#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#00FFFF", "#FF00FF",
  "#FFEFD5", "#0000CD", "#CD853F", "#D2B48C", "#66CDAA", "#000080", 
  "#008B8B", "#2E8B57", "#FFE4E1", "#6A5ACD", "#DDA0DD", "#E9967A", 
  "#A52A2A", "#FFFAFA", "#9370DB", "#DA70D6", "#4B0082", "#FFB6C1", 
  "#3CB371", "#FFEBCD", "#FFE4C4", "#DAA520", "#008080", "#BC8F8F", 
  "#FF69B4", "#FFDAB9", "#DEB887", "#7FFF00", "#8B4513", "#7CFC00", 
  "#FFFFE0", "#4682B4", "#006400", "#EE82EE", "#EEE8AA", "#F0FFF0", 
  "#F5DEB3", "#B8860B", "#20B2AA", "#FF1493", "#191970", "#708090", 
  "#228B22", "#F8F8FF", "#F5FFFA", "#FFA07A", "#90EE90", "#ADFF2F", 
  "#4169E1", "#FF6347", "#FAF0E6", "#800000", "#32CD32", "#F4A460", 
  "#FFFFF0", "#7B68EE", "#FFA500", "#ADD8E6", "#FFC0CB", "#7FFFD4", 
  "#FF8C00", "#8FBC8F", "#DC143C", "#FDF5E6", "#FFFAF0", "#00CED1", 
  "#00FF7F", "#800080", "#FFFACD", "#FA8072", "#9400D3", "#B22222", 
  "#FF7F50", "#87CEEB", "#6495ED", "#F0E68C", "#FAEBD7", "#FFF5EE", 
  "#6B8E23", "#87CEFA", "#00008B", "#8B008B", "#F5F5DC", "#BA55D3", 
  "#FFE4B5", "#FFDEAD", "#00BFFF", "#D2691E", "#FFF8DC", "#2F4F4F", 
  "#483D8B", "#AFEEEE", "#808000", "#B0E0E6", "#FFF0F5", "#8B0000", 
  "#F0FFFF", "#FFD700", "#D8BFD8", "#778899", "#DB7093", "#48D1CC", 
  "#FF00FF", "#C71585", "#9ACD32", "#BDB76B", "#F0F8FF", "#E6E6FA", 
  "#00FA9A", "#556B2F", "#40E0D0", "#9932CC", "#CD5C5C", "#FAFAD2", 
  "#5F9EA0", "#008000", "#FF4500", "#E0FFFF", "#B0C4DE", "#8A2BE2", 
  "#1E90FF", "#F08080", "#98FB98", "#A0522D"};

ColorLabelTable
::ColorLabelTable()
{
  // Copy default labels to active labels
  InitializeToDefaults();
}

void
ColorLabelTable
::LoadFromFile(const char *file)
  throw(itk::ExceptionObject)
{
  // Create a stream for reading the file
  ifstream fin(file);
  string line;

  // Create a temporary map of labels (to discard in case there is a problem
  // reading the file later)
  ValidLabelMap inputMap;

  // Set the clear label in the input map
  inputMap[0] = this->GetDefaultColorLabel(0);

  // Check that the file is readable
  if(!fin.good())
    {
    throw itk::ExceptionObject(
      __FILE__, __LINE__,"File does not exist or can not be opened");
    }

  // Read each line of the file separately
  for(unsigned int iLine=0;!fin.eof();iLine++)
    {
    // Read the line into a string
    std::getline(fin,line);

    // Check if the line is a comment or a blank line
    if(line[0] == '#' || line.length() == 0)
      continue;

    // Create a stream to parse that string
    IRISIStringStream iss(line);
    iss.exceptions(std::ios::badbit | std::ios::failbit);

    try 
      {
      // Read in the elements of the file
      LabelType idx;
      int red, green, blue, visible, mesh;
      float alpha;
      iss >> idx;
      iss >> red;
      iss >> green;
      iss >> blue;
      iss >> alpha;
      iss >> visible;
      iss >> mesh;

      // Skip to a quotation mark
      iss.ignore(line.length(),'\"');

      // Allocate a label of appropriate size
      char *label = new char[line.length()+1];

      // Read the label
      iss.get(label,line.length(),'\"');

      // Create a new color label
      ColorLabel cl;

      // Store the results
      // cl.SetValid(true);
      cl.SetRGB(0,(unsigned char) red);
      cl.SetRGB(1,(unsigned char) green);
      cl.SetRGB(2,(unsigned char) blue);
      cl.SetAlpha( (unsigned char) (255 * alpha) );
      cl.SetVisible(visible != 0);
      cl.SetVisibleIn3D(mesh != 0);
      cl.SetLabel(label);

      // Add the color label to the list, but not zero, because we can't allow
      // the clear label to be modified.
      if(idx > 0)
        inputMap[idx] = cl;

      // Clean up the label
      delete label;
      }
    catch( std::exception )
      {
      // Close the input stream
      fin.close();
      
      // create an exeption string
      IRISOStringStream oss;
      oss << "Syntax error on line " << (iLine+1);

      // throw our own exception
      throw itk::ExceptionObject(
        __FILE__, __LINE__,oss.str().c_str());
      }
    }  

  fin.close();

  // Use the labels that we have loaded
  m_LabelMap = inputMap;

  // Fire the event
  this->Modified();
  InvokeEvent(SegmentationLabelConfigurationChangeEvent());
}

void
ColorLabelTable
::SaveToFile(const char *file) const
  throw(itk::ExceptionObject)
{
  // Open the file for writing
  ofstream fout(file);
  
  // Check that the file is readable
  if(!fout.good())
    {
    throw itk::ExceptionObject(
      __FILE__, __LINE__,"File can not be opened for writing");
    }

  // Print out a header to the file
  fout << "################################################"<< endl;
  fout << "# ITK-SnAP Label Description File"               << endl;
  fout << "# File format: "                                 << endl;
  fout << "# IDX   -R-  -G-  -B-  -A--  VIS MSH  LABEL"     << endl;
  fout << "# Fields: "                                      << endl;
  fout << "#    IDX:   Zero-based index "                   << endl;
  fout << "#    -R-:   Red color component (0..255)"        << endl;
  fout << "#    -G-:   Green color component (0..255)"      << endl;
  fout << "#    -B-:   Blue color component (0..255)"       << endl;
  fout << "#    -A-:   Label transparency (0.00 .. 1.00)"   << endl;
  fout << "#    VIS:   Label visibility (0 or 1)"           << endl;
  fout << "#    IDX:   Label mesh visibility (0 or 1)"      << endl;
  fout << "#  LABEL:   Label description "                  << endl;
  fout << "################################################"<< endl;

  // Print out the labels
  for(ValidLabelMap::const_iterator it = m_LabelMap.begin();
      it != m_LabelMap.end(); it++)
    {
    LabelType index = it->first;
    const ColorLabel &cl = it->second;
    fout << "  "  << right << setw(3) << index;
    fout << "   " << right << setw(3) << (int) cl.GetRGB(0);
    fout << "  "  << right << setw(3) << (int) cl.GetRGB(1);
    fout << "  "  << right << setw(3) << (int) cl.GetRGB(2);
    fout << "  "  << right << setw(7)
      << setprecision(2) << (cl.GetAlpha() / 255.0f);
    fout << "  "  << right << setw(1) << (cl.IsVisible() ? 1 : 0);
    fout << "  "  << right << setw(1) << (cl.IsVisibleIn3D() ? 1 : 0);
    fout << "    \"" << cl.GetLabel() << "\"" << endl;
    }

  fout.close();
}   

void
ColorLabelTable
::LoadFromRegistry(Registry &registry)
{
  // Clear the table of labels, except the clear label
  this->RemoveAllLabels();

  // Read the number of color labels
  unsigned int nColorLabels = 
    registry["NumberOfElements"][0];

  // Read each label from the registry
  for(unsigned int i = 0; i < nColorLabels; i++) 
    {
    // Get the folder describing the label
    Registry &folder = registry.Folder(registry.Key("Element[%d]",i));

    // Get the index
    LabelType index = (LabelType) folder["Index"][0];

    // If index is valid, proceed to load the label
    if(index > 0 && index < MAX_COLOR_LABELS) 
      {
      // Get current color label 
      ColorLabel cl, def = this->GetDefaultColorLabel(index);

      // Read the color label properties
      cl.SetAlpha(folder["Alpha"][(int) def.GetAlpha()]);
      cl.SetLabel(folder["Label"][def.GetLabel()]);

      // Read the color property
      Vector3i color = 
        folder["Color"][Vector3i(def.GetRGB(0),def.GetRGB(1),def.GetRGB(2))];
      cl.SetRGB((unsigned char)color[0],(unsigned char)color[1],
                (unsigned char)color[2]);
      
      // Read the flag property
      Vector2i flags = folder["Flags"][Vector2i(0,0)];
      cl.SetVisibleIn3D(flags[0] > 0);
      cl.SetVisible(flags[1] > 0);
      // cl.SetValid(true);

      // Store
      m_LabelMap[index] = cl;
      }
    }

  // Fire the event
  this->Modified();
  InvokeEvent(SegmentationLabelConfigurationChangeEvent());
}

void
ColorLabelTable
::SaveToRegistry(Registry &registry) const
{
  // Write the labels themselves
  unsigned int validLabels = 0;

  for(ValidLabelMap::const_iterator it = m_LabelMap.begin();
      it != m_LabelMap.end(); it++)
    {
    // Get the i-th color label
    LabelType id = it->first;
    if(id > 0)
      {
      const ColorLabel &cl = it->second;

      // Create a folder for the color label
      Registry &folder =
        registry.Folder(registry.Key("Element[%d]",validLabels));

      folder["Index"] << (int) id;
      folder["Alpha"] << (int) cl.GetAlpha();
      folder["Label"] << cl.GetLabel();
      folder["Color"] << Vector3i(cl.GetRGB(0),cl.GetRGB(1),cl.GetRGB(2));
      folder["Flags"] << Vector2i(cl.IsVisibleIn3D(),cl.IsVisible());

      // Increment the valid label counter
      validLabels++;
      }
    }

  registry["NumberOfElements"] << validLabels;
}

void 
ColorLabelTable
::RemoveAllLabels()
{
  // Invalidate all the labels
  m_LabelMap.clear();
  m_LabelMap[0] = this->GetDefaultColorLabel(0);

  // Fire the event
  this->Modified();
  InvokeEvent(SegmentationLabelConfigurationChangeEvent());
}

void
ColorLabelTable
::InitializeToDefaults()
{
  // Remove all the labels
  m_LabelMap.clear();

  // Add the first six default labels
  for(LabelType l = 0; l <= NUM_INITIAL_COLOR_LABELS; l++)
    {
    m_LabelMap[l] = this->GetDefaultColorLabel(l);
    }

  // Fire the event
  this->Modified();
  InvokeEvent(SegmentationLabelConfigurationChangeEvent());
}


void 
ColorLabelTable
::SetColorLabelValid(LabelType id, bool flag)
{
  assert(id < MAX_COLOR_LABELS);

  ValidLabelMap::iterator it = m_LabelMap.find(id);
  
  if(flag && it == m_LabelMap.end())
    {
    // Label is being validated. If it does not exist, insert the default
    m_LabelMap[id] = this->GetDefaultColorLabel(id);
    this->Modified();
    InvokeEvent(SegmentationLabelConfigurationChangeEvent());
    }
  else if (!flag && it != m_LabelMap.end())
    {
    // The label is being invalidated - just delete it
    m_LabelMap.erase(it);
    this->Modified();
    InvokeEvent(SegmentationLabelConfigurationChangeEvent());
    }
}

bool
ColorLabelTable
::IsColorLabelValid(LabelType id) const
{
  assert(id < MAX_COLOR_LABELS);
  return (m_LabelMap.find(id) != m_LabelMap.end());
}


size_t 
ColorLabelTable
::GetNumberOfValidLabels() const
{
  return m_LabelMap.size();
}

ColorLabel ColorLabelTable::GetDefaultColorLabel(LabelType id)
{
  assert(id < MAX_COLOR_LABELS);

  ColorLabel deflab;

  // Special case of the clear label
  if(id == 0)
    {
    // Set up the clear color
    deflab.SetRGB(0,0,0);
    deflab.SetAlpha(0);
    // deflab.SetValid(true);
    deflab.SetVisible(false);
    deflab.SetVisibleIn3D(false);
    deflab.SetLabel("Clear Label");
    }
  else
    {
    unsigned char r = 0, g = 0, b = 0;
    parse_color(m_ColorList[(id-1) % m_ColorListSize],r,g,b);

    deflab.SetRGB(r,g,b);
    deflab.SetAlpha(255);
    // deflab.SetValid(true);
    deflab.SetVisible(true);
    deflab.SetVisibleIn3D(true);

    IRISOStringStream sout;
    sout << "Label " << id;
    deflab.SetLabel(sout.str().c_str());
    }

  return deflab;
}

void ColorLabelTable::SetColorLabel(size_t id, const ColorLabel &label)
{
  // Find the label
  ValidLabelMap::iterator it = m_LabelMap.find(id);

  // The current behavior is to make the label valid without the user explicitly
  // calling the SetValid method
  if(it == m_LabelMap.end())
    {
    m_LabelMap[id] = label;
    InvokeEvent(SegmentationLabelConfigurationChangeEvent());    
    }
  else
    {
    it->second = label;
    it->second.GetTimeStamp().Modified();
    InvokeEvent(SegmentationLabelPropertyChangeEvent());
    }



  this->Modified();
 }

const ColorLabel ColorLabelTable::GetColorLabel(size_t id) const
{
  // If the label exists, return it
  ValidLabelConstIterator it = m_LabelMap.find(id);
  if(it == m_LabelMap.end())
    return this->GetDefaultColorLabel(id);
  else
    return it->second;
}

LabelType ColorLabelTable::GetFirstValidLabel() const
{
  if(m_LabelMap.size() > 1)
    {
    ValidLabelConstIterator it = m_LabelMap.begin();
    it++;
    return it->first;
    }
  else return 0;
}

LabelType ColorLabelTable::GetInsertionSpot(LabelType pos)
{
  // Search for an available position
  for(int i = 0; i < MAX_COLOR_LABELS; i++)
    {
    LabelType testpos = (pos + i) % MAX_COLOR_LABELS;
    if(!this->IsColorLabelValid(testpos))
      return testpos;
    }

  // Nothing found - return 0
  return 0;
}

LabelType ColorLabelTable::FindNextValidLabel(
    LabelType pos, bool includeClearInSearch)
{
  // Search for an available position. This may be a little inefficient
  // but this is a rarely used operation.
  for(int i = 1; i < MAX_COLOR_LABELS; i++)
    {
    LabelType testpos = (pos + i) % MAX_COLOR_LABELS;
    if(IsColorLabelValid(testpos) && (testpos > 0 || includeClearInSearch))
      return testpos;
    }

  // Nothing found - return 0
  return 0;
}

