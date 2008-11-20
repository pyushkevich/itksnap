/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageIOWizardLogic.txx,v $
  Language:  C++
  Date:      $Date: 2008/11/20 04:23:28 $
  Version:   $Revision: 1.9 $
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
#ifndef __ImageIOWizardLogic_txx_
#define __ImageIOWizardLogic_txx_

#include "FL/Fl_File_Chooser.H"
#include "FL/Fl_Text_Buffer.H"
#include <stdio.h>
#include <cmath>
#include <map>
#include <string>
#include <iomanip>
#include <sstream>

#include "itkImage.h"
#include "itkImageIOBase.h"
#include "itkIOCommon.h"
#include "itkMetaDataObject.h"
#include "itkGDCMSeriesFileNames.h"
#include <itksys/SystemTools.hxx>

using std::map;
using namespace itk;

template <class TPixel>
ImageIOWizardLogic<TPixel>
::ImageIOWizardLogic() 
{                 

  // Initialize the DICOM directory lister
  m_DICOMLister = GDCMSeriesFileNames::New();
  
  // Initialize the text buffers
  m_SummaryTextBuffer = new Fl_Text_Buffer();

  // Initialize the callback pointer
  m_Callback = NULL;
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::MakeWindow()
{
  // Call the parent method
  ImageIOWizard::MakeWindow();

  // Initialize the file save dialog box based on the allowed file types
  for(unsigned int i = 0; i < GuidedImageIOBase::FORMAT_COUNT; i++)
    {
    // Create an appropriate description
    FileFormat fmt = static_cast<FileFormat>(i);
    FileFormatDescriptor fd = GuidedImageIOBase::GetFileFormatDescriptor(fmt);
    StringType text = fd.name + " File";
    
    // Add a menu option to the save menu, disabling it if it's unsupported
    m_InFilePageFormat->add(text.c_str(),0,NULL,NULL,
                            this->CanLoadFileFormat((FileFormat) i) ? 
                              0 : FL_MENU_INACTIVE);
    
    // Add a menu option to the save menu, disabling it if it's unsupported
    m_InSaveFilePageFormat->add(text.c_str(),0,NULL,NULL,
                                this->CanSaveFileFormat((FileFormat) i) ? 
                                  0 : FL_MENU_INACTIVE);
    }

  m_InSaveFilePageFormat->value(0);
  
  // Add the buffer to the summary text widget
  m_OutSummaryMetaData->buffer(m_SummaryTextBuffer);
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::GoBack(Fl_Group *current) 
{
  // The link to the last page
  Fl_Group *last = NULL;

  // In the input is NULL, get the current page from the wizard widget
  current = (current == NULL) ? (Fl_Group *)m_WizInput->value() : current;

  if (current == m_PageHeader || current == m_PageDICOM)
    {
    last = m_PageFile;
    }
  else if (current == m_PageSummary)
    {
    last = m_PageHeader;
    }
  else
    {
    assert(0 == "Next page not valid at this position in the wizard");
    }

  if (!last->active())
    {
    GoBack(last);
    } 
  else
    {
    m_WizInput->value(last);
    }
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::GoForward(Fl_Group *current) 
{
  // The link to the next page
  Fl_Group *next = NULL;

  // In the input is NULL, get the current page from the wizard widget
  current = (current == NULL) ? (Fl_Group *)m_WizInput->value() : current;

  // Follow the sequence of the pages
  if(current == m_PageFile) 
    {
    if(m_PageHeader->active())
      next = m_PageHeader;
    else if(m_PageDICOM->active())
      next = m_PageDICOM;
    else
      next = m_PageSummary;
    }
  else if (current == m_PageHeader || current == m_PageDICOM) 
    {
    next = m_PageSummary;
    }    
  else
    {
    assert(0 == "Next page not valid");
    }
    
  // Check if the page is active 
  if (!next->active())
    {
    GoForward(next);
    return;
    } 
    
  // Flip to the new page
  m_WizInput->value(next);

  // Call the enter-page method
  if (next == m_PageHeader)
    {
    OnHeaderPageEnter();
    }
  else if (next == m_PageDICOM)
    {
    OnDICOMPageEnter();
    }
  else if (next == m_PageSummary)
    {
    OnSummaryPageEnter();
    }
}

template <class TPixel>
typename ImageIOWizardLogic<TPixel>::StringType
ImageIOWizardLogic<TPixel>
::GetFilePattern(bool forLoading) 
{
  // String containing the whole patterns
  StringType pattern = "";
  bool patternNeedsTab = false;

  // String containing the "All Image Files" pattern
  StringType allImageFiles = "All Image Files (*.{"; 
  bool allImageFilesNeedsComma = false;

  // Go through all supported formats
  for(unsigned int i=0;i < GuidedImageIOBase::FORMAT_COUNT;i++)
    {
    FileFormat fmt = static_cast<FileFormat>(i);
    FileFormatDescriptor fd = GuidedImageIOBase::GetFileFormatDescriptor(fmt);

    // Check if the file format is supported
    if((forLoading && this->CanLoadFileFormat(fmt)) ||
       (!forLoading && this->CanSaveFileFormat(fmt)))
      {
      // Add comma to allImageFiles
      if(allImageFilesNeedsComma)
        allImageFiles += ",";
      else
        allImageFilesNeedsComma = true;

      // Add extension to all image files
      allImageFiles += fd.pattern;

      // Add a tab to the pattern
      if(patternNeedsTab)
        pattern += "\t";
      else
        patternNeedsTab = true;

      // Construct the pattern
      pattern += fd.name;
      pattern += " Files (*.{";
      pattern += fd.pattern;
      pattern += "})";
      }
    }

  // Finish the all image pattern
  allImageFiles += "})\t";

  // Compete the pattern
  pattern = allImageFiles + pattern;
  
  return pattern;
}


template <class TPixel>
typename ImageIOWizardLogic<TPixel>::FileFormat
ImageIOWizardLogic<TPixel>
::DetermineFileFormatFromFileName(bool forLoading, const char *testFile) 
{
  // Iterate over the known file types
  for(unsigned int i = 0;i < GuidedImageIOBase::FORMAT_COUNT;i++)
    {
    FileFormat fmt = static_cast<FileFormat>(i);
    FileFormatDescriptor fd = GuidedImageIOBase::GetFileFormatDescriptor(fmt);

    // Check if the file format is supported
    if((forLoading && this->CanLoadFileFormat(fmt)) ||
       (!forLoading && this->CanSaveFileFormat(fmt)))
      {
      // Create a matching pattern
      StringType pattern = "*.{" + fd.pattern + "}";

      // Check if the filename matches the pattern
      if(fl_filename_match(testFile,pattern.c_str()))
        return fmt;
      }
    }

  // Failed: return illegal pattern
  return GuidedImageIOBase::FORMAT_COUNT;
}

template <class TPixel>
bool ImageIOWizardLogic<TPixel>
::CanLoadFileFormat(FileFormat irisNotUsed(format)) const 
{ 
  return true; 
}

 
template <class TPixel>
bool ImageIOWizardLogic<TPixel>
::CanSaveFileFormat(FileFormat format) const 
{ 
  FileFormatDescriptor fd = 
    GuidedImageIOBase::GetFileFormatDescriptor(format); 
  return fd.can_write;
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::OnFilePageBrowse() 
{
  // Get the path and pattern for reading in the file 
  StringType pattern = this->GetFilePattern(true);
  
  const char *path = m_InFilePageBrowser->value();
  path = strlen(path) ? path : NULL;

  // Configure a file dialog
  char *fName = fl_file_chooser("Load Image", pattern.c_str(), path);
  
  // Bring up th choice dialog
  if (fName)
    {
    // Set the new filename
    m_InFilePageBrowser->value(fName);

    // Reset the format drop-down box to a null value
    m_InFilePageFormat->value(0);

    // Run the filename change event
    OnFilePageFileInputChange();
    }
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnFilePageFileFormatChange()
{
  // Activate the next button if there is a format selected
  if(m_InFilePageFormat->value() > 0)
    m_BtnFilePageNext->activate();
  else 
    m_BtnFilePageNext->deactivate();

  // If the user selects 'raw', update the format
  FileFormat format = (FileFormat) (m_InFilePageFormat->value() - 1);
  if (format == GuidedImageIOBase::FORMAT_RAW)
    m_PageHeader->activate();
  else
    m_PageHeader->deactivate();

  // If the user selects 'dicom' enable the dicom page
  if(format == GuidedImageIOBase::FORMAT_DICOM)
    m_PageDICOM->activate();
  else 
    m_PageDICOM->deactivate();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnFilePageFileHistoryChange()
{
  // Copy the history to the file box
  m_InFilePageBrowser->value(
    m_InFilePageHistory->mvalue()->label());

  // Update everything
  OnFilePageFileInputChange();
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::OnFilePageFileInputChange() 
{
  // Clear the registry
  m_Registry.Clear();

  // Check the length of the input
  const char *text = m_InFilePageBrowser->value();
  if (text != NULL && strlen(text) > 0)
    {
    // Try to load the registry associated with this filename
    if(m_Callback)
      m_Callback->FindRegistryAssociatedWithImage(
        m_InFilePageBrowser->value(), m_Registry);

    // If the registry contains a file format, override with that
    FileFormat fmt = 
      m_GuidedIO.GetFileFormat(m_Registry, GuidedImageIOBase::FORMAT_COUNT);

    // Try to select a file format accoring to the file name
    if(fmt == GuidedImageIOBase::FORMAT_COUNT)
      fmt = DetermineFileFormatFromFileName(true, text);
    
    // If the filename does not match any format, we do not change the 
    // format choice box in case that the user has already set it manually
    if(fmt < GuidedImageIOBase::FORMAT_COUNT)
      m_InFilePageFormat->value((int)fmt+1);
    
    // Run the format change event
    OnFilePageFileFormatChange();
    } 
  else
    { m_BtnFilePageNext->deactivate(); }            
}

template <class TPixel>
void ImageIOWizardLogic<TPixel>
::OnFilePageNext() 
{
  // Check if a file has been specified
  assert(m_InFilePageBrowser->value() 
    && strlen(m_InFilePageBrowser->value()) > 0);

  // Check that a format has been specified
  assert(m_InFilePageFormat->value() > 0);

  // Get the selected format and place it in the registry
  FileFormat format = (FileFormat) (m_InFilePageFormat->value() - 1);
  m_GuidedIO.SetFileFormat(m_Registry, format);

  // If file format is raw or dicom, go to the next page, o.w., load image
  switch(format) 
    {
    case GuidedImageIOBase::FORMAT_RAW:
      GoForward(); 
      break;
    case GuidedImageIOBase::FORMAT_DICOM:
      if(ProcessDICOMDirectory())
        GoForward();
      break;
    default:
      if(DoLoadImage())
        GoForward();
    }
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnCancel() 
{
  // Clear the data stored in the image
  // m_Image->Reset();
  m_Image = NULL;

  // Set the status to negative: nothing was loaded
  m_ImageLoaded = false;

  // Hide this window
  m_WinInput->hide();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnHeaderPageNext() 
{
  // Make sure we're not here by mistake
  assert(m_InFilePageFormat->value() - 1 == (int) GuidedImageIOBase::FORMAT_RAW);

  // Set up the registry with the specified values
  m_Registry["Raw.HeaderSize"] 
    << (unsigned int) m_InHeaderPageHeaderSize->value();

  // Set the dimensions
  m_Registry["Raw.Dimensions"] << Vector3i(
    (int) m_InHeaderPageDimX->value(),
    (int) m_InHeaderPageDimY->value(),
    (int) m_InHeaderPageDimZ->value());

  // Set the spacing
  m_Registry["Raw.Spacing"] << Vector3d(
    m_InHeaderPageSpacingX->value(),
    m_InHeaderPageSpacingY->value(),
    m_InHeaderPageSpacingZ->value());

  // Set the endianness
  m_Registry["Raw.BigEndian"] 
    << ( m_InHeaderPageByteAlign->value() == 0 );

  // Set the pixel type
  int iPixType = ((int)m_InHeaderPageVoxelType->value());
  GuidedImageIOBase::RawPixelType pixtype = (iPixType < 0) 
    ? GuidedImageIOBase::PIXELTYPE_COUNT
    : (GuidedImageIOBase::RawPixelType) iPixType;
  m_GuidedIO.SetPixelType(m_Registry, pixtype);

  // Do the loading
  if(DoLoadImage())
    GoForward();
}


template <class TPixel>
bool 
ImageIOWizardLogic<TPixel>
::ProcessDICOMDirectory()
{
  // Get the directory tree from the file
  StringType dirname = 
    itksys::SystemTools::FileIsDirectory(m_InFilePageBrowser->value())
    ? m_InFilePageBrowser->value()
    : itksys::SystemTools::GetParentDirectory(m_InFilePageBrowser->value());

  // Put up a wait cursor
  m_WinInput->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Try to list the DICOM files in the directory
  try
    {
    m_DICOMLister->SetDirectory(dirname.c_str());
    m_InFilePageBrowser->value(dirname.c_str());
    }
  catch(...)
    {
    fl_alert("Error listing DICOM files in directory: \n %s",dirname.c_str());
    }

  // Restore the cursor
  m_WinInput->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  
  // Get the list of series in the directory
  if(m_DICOMLister->GetSeriesUIDs().size() == 0)
    {
    fl_alert("No DICOM series found in the directory: \n%s",dirname.c_str());
    return false;
    }
  else return true;
 }

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnDICOMPageEnter()
{
  // Get the list of sequence ids
  const std::vector<StringType> &uids = m_DICOMLister->GetSeriesUIDs();

  // Add the ids to the menu
  m_InDICOMPageSequenceId->clear();
  for(unsigned int i = 0; i < uids.size(); i++)
    m_InDICOMPageSequenceId->add(uids[i].c_str());

  // See if one of the sequences in the registry matches
  StringType last = m_Registry["DICOM.SequenceId"]["NULL"];
  const Fl_Menu_Item *lastpos = m_InDICOMPageSequenceId->find_item(last.c_str());
  if(lastpos)
    m_InDICOMPageSequenceId->value(lastpos);
  else 
    m_InDICOMPageSequenceId->value(0);
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnDICOMPageNext()
{
  // The user will have selected some DICOM series. All we have to do is
  // specify the series in the registry and load the file
  m_Registry["DICOM.SequenceId"] << m_InDICOMPageSequenceId->mvalue()->label();

  // In addition, generate an explicit array of filenames for this sequence id. 
  // This will allow the DICOM loader to load files without having to list the 
  // directory again
  const std::vector<StringType> &files = m_DICOMLister->GetFileNames(
    StringType(m_InDICOMPageSequenceId->mvalue()->label()));
  m_Registry.Folder("DICOM.SliceFiles").PutArray(files);

  // Try loading the file now
  if(DoLoadImage())
    GoForward();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnDICOMPageBack()
{
  GoBack();
}



template <class TPixel>
bool 
ImageIOWizardLogic<TPixel>
::DoLoadImage()
{
  bool rc;

  // Show a wait cursor
  m_WinInput->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  
  // Try to load a file
  try 
    {     
    // Since we want to store the image IO, we need to use these two calls 
    // instead of just calling ReadImage with the registry
    m_Image = m_GuidedIO.ReadImage(
      m_InFilePageBrowser->value(), m_Registry, this->IsNativeFormatSupported());
    m_ImageIO = m_GuidedIO.GetIOBase();

    // Disconnect the image from the reader to free up memory (?)
    m_Image->DisconnectPipeline();

    // Check if the image is really valid
    rc = CheckImageValidity();
    }
  catch(ExceptionObject &exc)
  {
    // Clear the image and image IO
    m_Image = NULL;
    m_ImageIO = NULL;

    // Show the error
    fl_alert("Error reading image: %s.",exc.GetDescription());
    rc = false;
  }

  // Fix the cursor
  m_WinInput->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Check if the image is valid (subclasses can perform extra tasks here)
  return rc;
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnHeaderPageInputChange() 
{
  // Header input has changed
  if (m_InHeaderPageDimX->value() >= 1 &&
      m_InHeaderPageDimY->value() >= 1 &&
      m_InHeaderPageDimZ->value() >= 1 &&
      m_InHeaderPageSpacingX->value() > 0 &&
      m_InHeaderPageSpacingY->value() > 0 &&
      m_InHeaderPageSpacingZ->value() > 0)
    {
    m_BtnHeaderPageNext->activate();
    }
  else
    {
    m_BtnHeaderPageNext->deactivate();
    }
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnHeaderPageBack() 
{
  GoBack();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnFilePageEnter()
{

}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnHeaderPageEnter()
{
  // Use the values from the registry to set up the header page
  m_InHeaderPageHeaderSize->value(m_Registry["Raw.HeaderSize"][0]);
  
  // Set the dimensions
  Vector3i dims = m_Registry["Raw.Dimensions"][Vector3i(0)];
  m_InHeaderPageDimX->value(dims[0]);
  m_InHeaderPageDimY->value(dims[1]);
  m_InHeaderPageDimZ->value(dims[2]);

  // Set the spacing
  Vector3d spacing = m_Registry["Raw.Spacing"][Vector3d(1.0)];
  m_InHeaderPageSpacingX->value(spacing[0]);
  m_InHeaderPageSpacingY->value(spacing[1]);
  m_InHeaderPageSpacingZ->value(spacing[2]);

  // Set the data type
  RawPixelType pixtype = 
    m_GuidedIO.GetPixelType(m_Registry, GuidedImageIOBase::PIXELTYPE_UCHAR);
  m_InHeaderPageVoxelType->value((int)pixtype);

  // Set the endianness
  if(m_Registry["Raw.BigEndian"][true]) 
    m_InHeaderPageByteAlign->value(0);
  else
    m_InHeaderPageByteAlign->value(1);
}

template<class AnyType>
bool
try_print_metadata(std::ostream &sout, itk::MetaDataDictionary &mdd, std::string key)
  {
  AnyType value = 0;
  if(itk::ExposeMetaData<AnyType>(mdd, key, value))
    {
    sout << key << " = " << value << std::endl;
    return true;
    }
  else return false;
  }

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSummaryPageEnter()
{
  const char *boTypes[] = 
    {"Big Endian", "Little Endian","Order Not Applicable"};

  // Get the IO base object
  // itk::ImageIOBase *ioBase = m_Image->GetImageIO();

  // The object better not be NULL!
  if(m_ImageIO && m_Image)
    {
    // A stream to simplify converting to string
    IRISOStringStream sout;    

    // Print file name
    m_OutSummaryFileName->value(m_ImageIO->GetFileName());

    // Print file dimensions, spacing and origin
    for(size_t i = 0; i < 3; i++)
      {
      m_OutSummaryDimensions[i]->value(
        m_Image->GetBufferedRegion().GetSize()[i]);
      m_OutSummarySpacing[i]->value(
        m_Image->GetSpacing()[i]);
      m_OutSummaryOrigin[i]->value(
        m_Image->GetOrigin()[i]);
      }

    // Print file size in bytes
    m_OutSummarySize->value((int)(m_ImageIO->GetImageSizeInBytes() / (1024.0)));
    
    // Print out the orientation information
    sout.str("");
    vnl_matrix<double> dir = m_Image->GetDirection().GetVnlMatrix();
    std::string rai = 
      ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(dir);
    if(ImageCoordinateGeometry::IsDirectionMatrixOblique(dir))
      sout << "Oblique (closest to " << rai << ")";
    else
      sout << rai;
    m_OutSummaryOrientation->value(sout.str().c_str());
    
    // TODO: This is a workaround on an itk bug with RawImageIO
    if(m_ImageIO->GetComponentType() != ImageIOBase::UNKNOWNCOMPONENTTYPE)
      {
      // There actually is a type in the IO object
      m_OutSummaryPixelType->value(
        m_ImageIO->GetComponentTypeAsString(m_ImageIO->GetComponentType()).c_str());
      }
    else
      {
      m_OutSummaryPixelType->value(m_InHeaderPageVoxelType->text());
      }
    
    // Print the byte order
    m_OutSummaryByteOrder->value(
       boTypes[(unsigned int)(m_ImageIO->GetByteOrder() - ImageIOType::BigEndian)]);

    // Dump the contents of the meta data dictionary
    m_SummaryTextBuffer->text("");
    MetaDataDictionary &mdd = m_ImageIO->GetMetaDataDictionary();
    for(
      MetaDataDictionary::ConstIterator itMeta = mdd.Begin();
      itMeta != mdd.End(); ++itMeta)      
      {
      // Get the metadata as a generic object
      std::string key = itMeta->first, v_string;
      std::ostringstream sout;

      if(itk::ExposeMetaData<std::string>(mdd, key, v_string))
        {
        // For some weird reason, some of the strings returned by this method
        // contain '\0' characters. We will replace them by spaces
        std::ostringstream tmpout("");
        for(unsigned int i = 0; i < v_string.length(); i++)
          if(v_string[i] >= ' ') 
            tmpout << v_string[i];
        v_string = tmpout.str();

        // Make sure the value has more than blanks
        if(v_string.find_first_not_of(" ") != v_string.npos)
          sout << key << " = " << v_string << std::endl;
        }
      else 
        {
        bool rc = false;
        if(!rc) rc |= try_print_metadata<double>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<float>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<int>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<unsigned int>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<long>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<unsigned long>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<short>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<unsigned short>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<char>(sout, mdd, key);
        if(!rc) rc |= try_print_metadata<unsigned char>(sout, mdd, key);

        if(!rc)
          {
          sout << key << " of unsupported type " 
            << itMeta->second->GetMetaDataObjectTypeName() << std::endl;
          }
        }

      m_SummaryTextBuffer->append(sout.str().c_str());
      }
    }
  else 
    {
    m_OutSummaryFileName->value("Error loading image.");        
    m_OutSummarySize->value(0); 
    m_OutSummaryOrientation->value("n/a");
    m_OutSummaryPixelType->value("n/a");
    m_OutSummaryByteOrder->value("n/a");
    m_SummaryTextBuffer->text("");

    for(size_t i = 0; i < 3; i++)
      {
      m_OutSummaryDimensions[i]->value(0);
      m_OutSummarySpacing[i]->value(0);
      m_OutSummaryOrigin[i]->value(0);
      }
    }
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSummaryPageFinish() 
{
  // Perform a final validity / sanity check
  if (CheckFinalValidity()) 
  {
    // Set the status to positive, the image has been loaded!
    m_ImageLoaded = true;

    // Save the registry produced in this wizard
    if(m_Callback)
      m_Callback->UpdateRegistryAssociatedWithImage(
        m_InFilePageBrowser->value(), m_Registry);

    // Hide this window
    m_WinInput->hide();
  }
}

template <class TPixel>
ImageIOWizardLogic<TPixel>::~ImageIOWizardLogic() 
{
  delete m_SummaryTextBuffer;
}

template <class TPixel>
bool 
ImageIOWizardLogic<TPixel>
::DisplayInputWizard(const char *file) 
{
  // The loaded flag is false
  m_ImageLoaded = false;

  // Point the wizard to the first page
  m_WizInput->value(m_PageFile);

  // Clear the file name box
  // TODO: Implement a history list
  if(file)
    m_InFilePageBrowser->value(file);
  
  OnFilePageFileInputChange();

  // Show the input window
  m_WinInput->show();

  // Loop until the window has been closed
  while (m_WinInput->visible())
    Fl::wait();

  // Whether or not the load has been succesfull
  return m_ImageLoaded;
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSummaryPageBack() 
{
  GoBack();
}

template <class TPixel>
bool 
ImageIOWizardLogic<TPixel>
::DisplaySaveWizard(ImageType *image,const char *file)
{
  // Clear the saved flag
  m_ImageSaved = false;

  // Store the image
  m_Image = image;

  // Clear the file name box
  // TODO: Implement a history list
  if(file)
    m_InFilePageBrowser->value(file);
  OnSaveFilePageFileInputChange();

  // Show the input window
  m_WinOutput->show();

  // Loop until the window has been closed
  while (m_WinOutput->visible())
    Fl::wait();

  // Clear the image pointer
  m_Image = NULL;

  // Whether or not the load has been succesfull
  return m_ImageSaved;
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveFilePageFileInputChange()
{
  // Clear the registry
  m_Registry.Clear();

  // Check the length of the input
  const char *text = m_InSaveFilePageBrowser->value();
  if (text != NULL && strlen(text) > 0)
    {
    // Try to load the registry associated with this filename
    if(m_Callback)
      m_Callback->FindRegistryAssociatedWithImage(
        m_InSaveFilePageBrowser->value(), m_Registry);

    // If the registry contains a file format, override with that
    FileFormat fmt = 
      m_GuidedIO.GetFileFormat(m_Registry, GuidedImageIOBase::FORMAT_COUNT);

    // Try to select a file format accoring to the file name
    if(fmt == GuidedImageIOBase::FORMAT_COUNT)
      fmt = DetermineFileFormatFromFileName(true, text);

    // If the filename does not match any format, we do not change the 
    // format choice box in case that the user has already set it manually
    if(fmt < GuidedImageIOBase::FORMAT_COUNT)
      m_InSaveFilePageFormat->value((int)fmt+1);
    
    // Run the format change event
    OnSaveFilePageFileFormatChange();
    } 
  else
    { m_BtnSaveFilePageNext->deactivate(); }            
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveFilePageFileHistoryChange()
{
  // Copy the history to the file box
  m_InSaveFilePageBrowser->value(
    m_InSaveFilePageHistory->mvalue()->label());

  // Update everything
  OnSaveFilePageFileInputChange();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveFilePageFileFormatChange()
{
  // Activate the next button if there is a format selected
  if(m_InSaveFilePageFormat->value() > 0)
    m_BtnSaveFilePageNext->activate();
  else 
    m_BtnSaveFilePageNext->deactivate();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveFilePageBrowse()
{  
  // Get the path and pattern for reading in the file 
  const char *path = m_InSaveFilePageBrowser->value();
  path = strlen(path) ? path : NULL;

  // Get a pattern
  StringType pattern = this->GetFilePattern(false);

  // Create a file chooser
  char *fName = fl_file_chooser("Save Image As", pattern.c_str(), path);
  if (fName)
    {
    // Set the new filename
    m_InSaveFilePageBrowser->value(fName);

    // Reset the format drop-down box to a null value
    m_InSaveFilePageFormat->value(0);

    // Run the filename change event
    OnSaveFilePageFileInputChange();
    }
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveFilePageSave()
{
  // There better be a format selected
  assert(m_InSaveFilePageFormat->value() > 0);

  // Get the selected format and place it in the registry
  FileFormat format = (FileFormat) (m_InSaveFilePageFormat->value() - 1);
  m_GuidedIO.SetFileFormat(m_Registry, format);

  // Put up a waiting cursor
  m_WinOutput->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Try to save the image using the current format
  try 
    {
    // Try to save the image
    m_GuidedIO.SaveImage(
      m_InSaveFilePageBrowser->value(), m_Registry, m_Image);
    m_ImageSaved = true;

    // Save the registry produced in this wizard
    if(m_Callback)
      m_Callback->UpdateRegistryAssociatedWithImage(
        m_InFilePageBrowser->value(), m_Registry);

    // Hide the dialog
    m_WinOutput->hide();
    }
  catch(ExceptionObject &exc)
    { fl_alert("Error saving file: %s",exc.GetDescription()); }
  
  // Restore the cursor
  m_WinOutput->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::OnSaveCancel()
{
  // Just close the window
  m_WinOutput->hide();
}

template <class TPixel>
void 
ImageIOWizardLogic<TPixel>
::SetHistory(const HistoryType &history)
{
  // Store the history
  m_History = history;

  // Clear the history drop box
  m_InFilePageHistory->clear();
  m_InSaveFilePageHistory->clear();

  // Add the history
  if(history.size() > 0)
    {  
    // Add each item to the history menu (history is traversed
    // backwards)
    for(HistoryType::reverse_iterator it=m_History.rbegin();
        it!=m_History.rend();it++)
      {
      // FLTK's add() treats slashes as submenu separators, hence this code
      m_InFilePageHistory->replace(
        m_InFilePageHistory->add("dummy"),it->c_str());      
      m_InSaveFilePageHistory->replace(
        m_InSaveFilePageHistory->add("dummy"),it->c_str());
      }

    // Activate the history menu    
    m_InFilePageHistory->activate();
    m_InSaveFilePageHistory->activate();
    }
  else
    {
    // Deactivate history
    m_InFilePageHistory->deactivate();
    m_InSaveFilePageHistory->deactivate();
    }
}


#endif // __ImageIOWizardLogic_txx_
