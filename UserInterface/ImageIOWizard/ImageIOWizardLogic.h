/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:16 $
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
#ifndef __ImageIOWizardLogic_h_
#define __ImageIOWizardLogic_h_

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#endif

#include <assert.h>
#include <string>
#include <vector>

#include "ImageIOWizard.h"
#include "FL/fl_draw.H"
#include "itkSmartPointer.h"
#include "ImageCoordinateGeometry.h"
#include "Registry.h"
#include <string>
#include <vector>

#include "GuidedImageIO.h"

class Fl_Text_Buffer;
namespace itk 
{
  template <class TPixel,unsigned int VDimensions> class Image;
  template <class TPixel,unsigned int VDimensions> class RAWImageIO;
  template <unsigned int VDimensions> class Size;
  template <unsigned int VDimensions> class ImageBase;
  class ImageIOBase;  
  class GDCMSeriesFileNames;
}

/**
 * \class ImageInfoCallbackInterface
 * \brief A virtual class that is used to provide image-specific information to 
 * the ImageIOWizardLogic object 
 */
class ImageInfoCallbackInterface
{
public:
    virtual ~ImageInfoCallbackInterface() {}
  virtual bool FindRegistryAssociatedWithImage(
    const char *file, Registry &registry) = 0;

  virtual void UpdateRegistryAssociatedWithImage(
    const char *file, Registry &registry) = 0;
};


/**
 * \class ImageIOWizardLogic
 * The implementation of the Image IO Wizard UI class.  This class defines the
 * callbacks in the user interface for the class.  It is templated over the type
 * of the input that is to be procured.
 */
template <class TPixel>
class ImageIOWizardLogic : public ImageIOWizard 
{
public:
  // typedef ImageWrapper<TPixel> WrapperType;

  // Image type definition
  typedef itk::Image<TPixel, 3> ImageType;
  typedef typename itk::SmartPointer<ImageType> ImagePointer;

  // Image IO type definition
  typedef itk::ImageIOBase ImageIOType;
  typedef typename itk::SmartPointer<ImageIOType> ImageIOPointer;

  // Other typedefs
  typedef std::string StringType;
  typedef std::vector<StringType> HistoryType;

  /** A constructor */
  ImageIOWizardLogic();

  /** A descructor */
  virtual ~ImageIOWizardLogic();

  // Callback methods overridden from the Base class
  virtual void OnCancel();
  virtual void OnFilePageBrowse();
  virtual void OnFilePageNext();
  virtual void OnFilePageFileInputChange();
  virtual void OnFilePageFileFormatChange();
  virtual void OnFilePageFileHistoryChange();
  virtual void OnHeaderPageNext();
  virtual void OnHeaderPageBack();
  virtual void OnHeaderPageInputChange();  
  virtual void OnDICOMPageNext();
  virtual void OnDICOMPageBack();
  virtual void OnOrientationPageNext();
  virtual void OnOrientationPageBack();
  virtual void OnOrientationPageSelectPreset();
  virtual void OnOrientationPageSelect();
  virtual void OnOrientationPageRAIChange();
  virtual void OnSummaryPageFinish();
  virtual void OnSummaryPageBack();

  // Functions called on entering wizard input pages
  virtual void OnFilePageEnter();
  virtual void OnHeaderPageEnter();
  virtual void OnDICOMPageEnter();
  virtual void OnOrientationPageEnter();
  virtual void OnSummaryPageEnter();

  // Save related functions
  virtual void OnSaveFilePageFileInputChange();
  virtual void OnSaveFilePageFileFormatChange();
  virtual void OnSaveFilePageFileHistoryChange();
  virtual void OnSaveFilePageBrowse();
  virtual void OnSaveFilePageSave();
  virtual void OnSaveCancel();

  // Custom initialization code
  virtual void MakeWindow();

  /**
   * Get the image that has been loaded by this object
   */
  ImageType *GetLoadedImage() 
  {
    assert(IsImageLoaded());
    return m_Image;
  }
  
  /**
   * Get the RAI orientation code read from the image or supplied by the
   * user.  Returns NULL if RAI is invalid
   */
  const char *GetLoadedImageRAI()
  {
    const char *rai = m_InRAICode->value();
    return ImageCoordinateGeometry::IsRAICodeValid(rai) ? rai : NULL;
  }
  
  
  /**
   * Has the image been loaded successfully?
   */
  bool IsImageLoaded() 
  {
    return m_ImageLoaded;
  }

  /**
   * Unload the image to free memory (do this after retrieving the image for 
   * use elsewhere)
   */
  void ReleaseImage()
  {
    m_Image = NULL;
    m_ImageLoaded = false;
  }

  /**
   * A method to initialize the pages in the wizard.  Child classes should override
   * this and enable/disable irrelevant pages and fields.  The method takes as a 
   * parameter an ImageWrapper, to which it makes changes.
   *
   * The method returns true if an image was loaded with success, false in case of cancellation
   */
  virtual bool DisplayInputWizard(const char *file);   

  /**
   * A method to save an image using the wizard (at this point it's just a one
   * page wizard 
   */
  virtual bool DisplaySaveWizard(ImageType *image, const char *file);

  /**
   * Get the filename that was loaded
   */
  const char *GetFileName() 
  {
    return m_InFilePageBrowser->value();
  }

  /** Get the filename that was saved */
  const char *GetSaveFileName()
  {
    return m_InSaveFilePageBrowser->value();
  }

  /** Set the history list of recently opened files */
  void SetHistory(const HistoryType &history);

  /** Set the callback object for retrieving historical information about
   * previously loaded images */
  void SetImageInfoCallback(ImageInfoCallbackInterface *iCallback)
    { this->m_Callback = iCallback; }

protected:

  /** An image being loaded */
  ImagePointer m_Image;

  /** The image IO mechanism */
  ImageIOPointer m_ImageIO;

  /** Has the image been loaded? */
  bool m_ImageLoaded;

  /** Has the image been saved? */
  bool m_ImageSaved;

  /** A history of recently opened files */
  HistoryType m_History;

  /** A callback for retrieving image information */
  ImageInfoCallbackInterface *m_Callback;

  /** DICOM file names lister */
  typename itk::SmartPointer<itk::GDCMSeriesFileNames> m_DICOMLister;

  /** The registry associated with the image currently being considered for loading */
  Registry m_Registry;

  /** A guided image IO object used to load images */
  GuidedImageIO<TPixel> m_GuidedIO;

  /** A mapping from axis index and flip state to orientation menu items */
  unsigned int m_MapOrientationIndexAndFlipToMenuItem[3][2];
  
  /** A mapping from axis orientation index to doll wizard page */
  unsigned int m_MapOrientationIndexToDollPageIndex[3][3][3];

  /** A mapping from axis orientation index to position of origin 
   * on the doll image */
  unsigned int m_OrientationIndexToDollVertex[3][3][3];

  /** Text buffer for one of the summary widgets */
  Fl_Text_Buffer *m_SummaryTextBuffer;

  // -- Stuff dealing with file formats --
  typedef GuidedImageIOBase::FileFormat FileFormat;
  typedef GuidedImageIOBase::RawPixelType RawPixelType;

  /** Extensions for different file formats */
  StringType m_FileFormatPattern[GuidedImageIOBase::FORMAT_COUNT];

  /** Brief descriptions of different file formats */
  StringType m_FileFormatDescription[GuidedImageIOBase::FORMAT_COUNT];  
  
  /**
   * Allow children to specify which file formats they can and can't save
   */
  virtual bool CanSaveFileFormat(FileFormat format) const; 

  /**
   * Allow children to specify which file formats they can and can't load
   */
  virtual bool CanLoadFileFormat(FileFormat format) const;

  /**
   * This method should return a string containing the patterns of files
   * that this dialog is allowed to read
   */
  StringType GetFilePattern(bool forLoading);

  /**
   * This method checks which of the supported file formats matches a filename
   * entered by the user
   */
  FileFormat DetermineFileFormatFromFileName(bool forLoading, 
                                             const char *testFileName);

  /**
   * A method that controls how we propagate forward and back in the dialog.
   * Fires an assertion if called from where you can't go back or forward.  Inactive
   * pages are simply skipped.
   */
  void GoForward(Fl_Group *current = NULL);
  void GoBack(Fl_Group *current = NULL);

  // This is called to load the image with a possible custom IO for raw images
  bool DoLoadImage();

  // List files in the dicom directory
  bool ProcessDICOMDirectory();

  // Check image validity after the initial load
  virtual bool CheckImageValidity() { return true; }

  // Try to determine the RAI code of the image
  virtual void GuessImageOrientation();

  // Check if the image is valid
  virtual bool CheckFinalValidity() 
  {
    return (m_Image);
  }

  // Apply a new RAI structure
  void SetRAI(const char *);
  
  // Set the state reflecting an invalid RAI
  void SetRAIToInvalid(const char *);
};

// TXX not included on purpose!

#endif // __ImageIOWizardLogic_h_
