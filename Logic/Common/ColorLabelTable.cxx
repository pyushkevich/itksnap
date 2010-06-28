#include "ColorLabelTable.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace std;

extern int fl_parse_color(
  const char* p, unsigned char& r, unsigned char& g, unsigned char& b);

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
  // Initialize the default labels
  const unsigned int INIT_VALID_LABELS = 7;

  // Set up the clear color
  m_DefaultLabel[0].SetRGB(0,0,0);
  m_DefaultLabel[0].SetAlpha(0);
  m_DefaultLabel[0].SetValid(true);
  m_DefaultLabel[0].SetVisible(false);
  m_DefaultLabel[0].SetVisibleIn3D(false);
  m_DefaultLabel[0].SetLabel("Clear Label");

  // Set the colors from the table
  for(size_t i = 1; i < MAX_COLOR_LABELS; i++)
    {
    unsigned char r,g,b;
    fl_parse_color(m_ColorList[(i-1) % m_ColorListSize],r,g,b);
    
    m_DefaultLabel[i].SetRGB(r,g,b);
    m_DefaultLabel[i].SetAlpha(255);
    m_DefaultLabel[i].SetValid(i < INIT_VALID_LABELS);
    m_DefaultLabel[i].SetVisible(true);
    m_DefaultLabel[i].SetVisibleIn3D(true);

    IRISOStringStream sout;
    sout << "Label " << i;
    m_DefaultLabel[i].SetLabel(sout.str().c_str());
    }

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

  // Create a temporary array of color labels
  vector<ColorLabel> xTempLabels;
  vector<size_t> xTempLabelIds;

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
      int idx, red, green, blue, visible, mesh;
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
      cl.SetValid(true);
      cl.SetRGB(0,(unsigned char) red);
      cl.SetRGB(1,(unsigned char) green);
      cl.SetRGB(2,(unsigned char) blue);
      cl.SetAlpha( (unsigned char) (255 * alpha) );
      cl.SetVisible(visible != 0);
      cl.SetVisibleIn3D(mesh != 0);
      cl.SetLabel(label);

      // Add the color label to the list
      xTempLabels.push_back(cl);
      xTempLabelIds.push_back(idx);

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

  // Ok, we should have a list of color labels. Now initalize the color label array
  // the blank state
  RemoveAllLabels();

  // Now, add all the labels
  for(size_t iLabel = 0; iLabel < xTempLabels.size(); iLabel++)
    if(xTempLabelIds[iLabel] > 0)
      SetColorLabel(xTempLabelIds[iLabel], xTempLabels[iLabel]);
}


void
ColorLabelTable
::SaveToFile(const char *file)
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
  for(unsigned int i=0;i<MAX_COLOR_LABELS;i++)
    {
    const ColorLabel &cl = GetColorLabel(i);
    if(cl.IsValid())
      {
      fout << "  "  << right << setw(3) << i;
      fout << "   " << right << setw(3) << (int) cl.GetRGB(0);
      fout << "  "  << right << setw(3) << (int) cl.GetRGB(1);
      fout << "  "  << right << setw(3) << (int) cl.GetRGB(2);
      fout << "  "  << right << setw(7) 
        << setprecision(2) << (cl.GetAlpha() / 255.0f);
      fout << "  "  << right << setw(1) << (cl.IsVisible() ? 1 : 0);
      fout << "  "  << right << setw(1) << (cl.IsVisibleIn3D() ? 1 : 0);
      fout << "    \"" << cl.GetLabel() << "\"" << endl;
      }
    }

  fout.close();
}   

void
ColorLabelTable
::LoadFromRegistry(Registry &registry)
{
  // Clear the table of labels
  RemoveAllLabels();

  // Read the number of color labels
  unsigned int nColorLabels = 
    registry["NumberOfElements"][0];

  // Read each label from the registry
  for(unsigned int i = 0; i < nColorLabels; i++) 
    {
    // Get the folder describing the label
    Registry &folder = registry.Folder(registry.Key("Element[%d]",i));

    // Get the index
    int index = folder["Index"][0];

    // If index is valid, proceed to load the label
    if(index > 0 && index < MAX_COLOR_LABELS) 
      {
      // Get current color label 
      ColorLabel &cl = m_Label[index];

      // Read the color label properties
      cl.SetAlpha(folder["Alpha"][(int) cl.GetAlpha()]);
      cl.SetLabel(folder["Label"][cl.GetLabel()]);
      
      // Read the color property
      Vector3i color = 
        folder["Color"][Vector3i(cl.GetRGB(0),cl.GetRGB(1),cl.GetRGB(2))];
      cl.SetRGB((unsigned char)color[0],(unsigned char)color[1],
                (unsigned char)color[2]);
      
      // Read the flag property
      Vector2i flags = folder["Flags"][Vector2i(0,0)];
      cl.SetVisibleIn3D(flags[0] > 0);
      cl.SetVisible(flags[1] > 0);
      cl.SetValid(true);
      }
    }
}

void
ColorLabelTable
::SaveToRegistry(Registry &registry)
{
  // Write the labels themselves
  unsigned int validLabels = 0;
  for(unsigned int i=1;i < MAX_COLOR_LABELS; i++)
    {
    // Get the i-th color label
    const ColorLabel &cl = m_Label[i];
    
    // Only write valid color labels (otherwise, what's the point?)
    if(!cl.IsValid()) continue;
    
    // Create a folder for the color label
    Registry &folder = 
      registry.Folder(registry.Key("Element[%d]",validLabels));    
    
    folder["Index"] << i;
    folder["Alpha"] << (int) cl.GetAlpha();
    folder["Label"] << cl.GetLabel();
    folder["Color"] << Vector3i(cl.GetRGB(0),cl.GetRGB(1),cl.GetRGB(2));
    folder["Flags"] << Vector2i(cl.IsVisibleIn3D(),cl.IsVisible());

    // Increment the valid label counter
    validLabels++;
    }

  registry["NumberOfElements"] << validLabels;
}

void 
ColorLabelTable
::RemoveAllLabels()
{
  // Invalidate all the labels
  for(size_t iLabel = 1; iLabel < MAX_COLOR_LABELS; iLabel++)
    { m_Label[iLabel].SetValid(false); }
}

size_t
ColorLabelTable
::GetFirstValidLabel() const
{
  for(size_t iLabel = 1; iLabel < MAX_COLOR_LABELS; iLabel++)
    if(m_Label[iLabel].IsValid())
      return iLabel;
  return 0;
}

void
ColorLabelTable
::InitializeToDefaults()
{
  // Set the properties of all non-clear labels
  for (size_t i=0; i<MAX_COLOR_LABELS; i++)
    m_Label[i] = m_DefaultLabel[i];
}


void 
ColorLabelTable
::SetColorLabelValid(size_t id, bool flag)
  {
  assert(id < MAX_COLOR_LABELS);
  
  // Labels that are invalidated are reset to defaults
  if(!flag && m_Label[id].IsValid())
    m_Label[id] = m_DefaultLabel[id];

  // Set the flag
  m_Label[id].SetValid(flag); 
  }


size_t 
ColorLabelTable
::GetNumberOfValidLabels()
{
  size_t n = 0;
  for(size_t i = 0; i < MAX_COLOR_LABELS; i++)
    if(m_Label[i].IsValid())
      ++n;
  return n;
}

