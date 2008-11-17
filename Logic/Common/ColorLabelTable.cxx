#include "ColorLabelTable.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace std;
using namespace itk;

ColorLabelTable
::ColorLabelTable()
{
  // Initialize the default labels
  unsigned int i;
  const unsigned int INIT_VALID_LABELS = 7;

  // Set up the clear color
  m_DefaultLabel[0].SetRGB(0,0,0);
  m_DefaultLabel[0].SetAlpha(0);
  m_DefaultLabel[0].SetValid(true);
  m_DefaultLabel[0].SetVisible(false);
  m_DefaultLabel[0].SetVisibleIn3D(false);
  m_DefaultLabel[0].SetLabel("Clear Label");

  // Some well-spaced sample colors to work with
  m_DefaultLabel[1].SetRGB(255,0,0);
  m_DefaultLabel[2].SetRGB(0,255,0);
  m_DefaultLabel[3].SetRGB(0,0,255);
  m_DefaultLabel[4].SetRGB(255,255,0);
  m_DefaultLabel[5].SetRGB(0,255,255);
  m_DefaultLabel[6].SetRGB(255,0,255);

  // Fill the rest of the labels with color ramps
  for (i = INIT_VALID_LABELS; i < MAX_COLOR_LABELS; i++)
    {
    if (i < 85)
      {
      m_DefaultLabel[i].SetRGB(0,(unsigned char) ((84.0-i)/85.0 * 200.0 + 50));
      m_DefaultLabel[i].SetRGB(1,(unsigned char) (i/85.0 * 200.0 + 50));
      m_DefaultLabel[i].SetRGB(2,0);
      }
    else if (i < 170)
      {
      m_DefaultLabel[i].SetRGB(0,0);
      m_DefaultLabel[i].SetRGB(1,(unsigned char) ((169.0-i)/85.0 * 200.0 + 50));
      m_DefaultLabel[i].SetRGB(2,(unsigned char) ((i-85)/85.0 * 200.0 + 50));
      }
    else
      {
      m_DefaultLabel[i].SetRGB(0,(unsigned char) ((i-170)/85.0 * 200.0 + 50));
      m_DefaultLabel[i].SetRGB(1,0);
      m_DefaultLabel[i].SetRGB(2,(unsigned char) ((255.0-i)/85.0 * 200.0 + 50));
      }
    }

  // Set the properties of all non-clear labels
  for (i=1; i<MAX_COLOR_LABELS; i++)
    {
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
  throw(ExceptionObject)
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
  throw(ExceptionObject)
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

