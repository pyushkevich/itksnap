/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: TutorialTest.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:20 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include <SNAPBorlandDummyTypes.h>
#endif

#include "ImageIORoutines.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "LevelSetMeshPipeline.h"
#include "SNAPImageData.h"

#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"

#include <vector>

/** 
 * This function is designed to serve as a tutorial through the classes 
 * that form the logical framework of SNAP. In this test, the student will
 * be able to load images, perform preprocessing, initialize and run segmentation,
 * output slices from the segmentation and relabel the segmentatinon
 *
 * The method should be invoked with two parameters. First is the full path to
 * the file MRIcrop-orig.gipl which contains the test image. Second is the full
 * path to the label file MRIcrop-seg.label. Both of these files can be downloaded
 * from the SNAP website 
 */
int main(int argc, char *argv[]) 
{
  if(argc < 3)
    {
    cerr << "Must specify two arguments." << endl;
    return 1;
    }
  /* =======================================================================
   * Section 1. Starting the Application and Loading an Image
   * ====================================================================== */

  /* We begin by creating an IRISApplication object. This is the most high
   * level object in the SNAP class hierarchy and it provides access to the 
   * remainder of the SNAP logic classes */
  IRISApplication application;

  /* Next we will load the test image. In this example it is expected that the
   * image MRIcrop-orig.gipl, which can be downloaded from the SNAP website 
   * will be used. The user will pass the full path to this file as the first
   * parameter to the program */
  const char *sImageFileName = argv[1];
  
  try {
    /* The responsibility for loading the image is with the user. First, we 
     * create an itk::Image object into which the image will be loaded from
     * disk */
    IRISApplication::GreyImageType::Pointer imgGrey;

    /* SNAP provides a helper method LoadImageFromFile for loading images */ 
    LoadImageFromFile(sImageFileName,imgGrey);

    /* Once the image has been loaded, it can be passed on to the IRISApplication.
     * However, SNAP needs to know the orientation of the image, which is 
     * expressed as the position of the image's origin {X,Y,Z}={0,0,0} in 
     * patient coordinates. This information is passed on in the form of a 
     * three letter RAI code. The value "ASR" means that the origin lies in the
     * Anterior-Superior-Right corner of the patient coordinate system, with
     * X axis corresponding to the Anterior-Posterior axis, Y axis corresponding
     * to the Inferior-Superior axis and Z to the Right-Left axis. For the image
     * we are loading, the correct code is "RAI" */
    application.UpdateIRISGreyImage(imgGrey,"RAI");

    /* In addition to loading the image, we are going to load the segmentation
     * labels associated with this image. Segmentation labels are stored in the 
     * file MRIcrop-seg.label and passed on as an argument to this test program */
    const char *sLabelFileName = argv[2];
    application.GetColorLabelTable()->LoadFromFile(sLabelFileName);
  }
  catch(itk::ExceptionObject &exception)
  {
    // This code is called if LoadImageFromFile fails.
    cout << "Error Loading Image: " << exception;
    return -1;
  }

  /* =======================================================================
   * Section 2. Accessing Image Data Using IRISImageData Class
   * ====================================================================== */
                                      
  /* Now the image has been loaded into IRISApplication. IRISApplication stores
   * its data using two high level classes: IRISImageData and SNAPImageData. 
   * IRISImageData holds the original greyscale image that we have loaded as
   * well as the labelling of that image that is constructed throughout the 
   * segmentation. SNAPImageData is used for automatic segmentation and contains
   * a subregion of the original greyscale image. At this point SNAPImageData has
   * not yet been allocated. SNAPImageData is a child class of IRISImageData.
   *
   * The method IRISApplication::GetCurrentImageData() returns the IRISImageData
   * object associated with the current state of SNAP. In manual state (current
   * state right now) it returns a pointer to an IRISImageData object. In
   * automatic segmentation state, it returns a pointer to a SNAPImageData 
   * object.
   *
   * The IRISImageData and SNAPImageData objects can also be accessed explicitly 
   * using the GetIRISImageData() and GetSNAPImageData() pointers.
   *
   * We will begin by examining the IRISImageData class. We can use it to access
   * the underlying image and to examine the segmentation. */
  IRISImageData *irisData = application.GetCurrentImageData();

  /* IRISImageData contains two images: the grey level image and a label or 
   * segmentation image. Both images are of the same dimensions. The grey image
   * has voxels of type GreyType and the segmentation image of type LabelType */
  cout << "Image Dimensions: " << 
    to_int(irisData->GetVolumeExtents()) << endl;       // Ignore the to_int!
//  cout << "Voxel Size      : " << irisData->GetVoxelScaleFactor() << endl;

  /* IRISImageData also handles segmentation labels. We can inquire about each label. 
   * Labels are indexed from 1 to 255 (0 is the clear label). Only some labels are 
   * defined as 'valid', i.e., available to the user to edit */
  for(unsigned int iLabel=0; iLabel < MAX_COLOR_LABELS; iLabel++)
    {
    /* Labels are represented by the class ColorLabel. Each label has a color,
     * a transparency level, a visible flag, a 'visible in 3d mesh flag', and a
     * textual description */
    const ColorLabel &label = application.GetColorLabelTable()->GetColorLabel(iLabel);                                                       
    cout << "Label Number  : " << iLabel << endl;
    cout << "  Decription  : " << label.GetLabel() << endl;
    cout << "  Color       : {" 
      << (int) label.GetRGB(0) << "," 
      << (int) label.GetRGB(1) << "," 
      << (int) label.GetRGB(2) << "}" << endl;
    cout << "  Opacity     : " << label.GetAlpha() << endl;
    }

  /* The current drawing label and the current draw-over label are properties of
   * the SNAP application and are accessed using the GlobalState class.
   * GlobalState stores a great number of application-wide settings */
  application.GetGlobalState()->SetDrawingColorLabel(7);  // caudate label
  application.GetGlobalState()->SetCoverageMode(PAINT_OVER_ALL);

  /* The next class we will learn about is the ImageWrapper class, which is a
   * wrapper around the standard itk::Image class. The ImageWrapper provides
   * several mini-pipelines involving the itk::Image stored within. First
   * let's access the grey images' wrapper in IRISImageData */
  GreyImageWrapper *wrapGrey = irisData->GetGrey();

  /* The class GreyImageWrapper is a subclass of the generic templated class 
   * ImageWrapper<TPixel> with template argument TPixel=GreyType. We can
   * use ImageWrapper to get to the itk::Image itself */
  GreyImageWrapper::ImagePointer imgGrey = wrapGrey->GetImage();

  /* Here are some of the methods available through the ImageWrapper class */
  cout << "Grey Min        : " << wrapGrey->GetImageMin() << endl;
  cout << "Grey Max        : " << wrapGrey->GetImageMax() << endl;

  /* The IRISImageData and ImageWrapper classes keep track of the current 
   * position of the SNAP 3D cursor. When an image is loaded, the cursor is 
   * positioned in the center of the image, as we can see by querying the 
   * ImageWrapper class */
  Vector3ui vCursor = wrapGrey->GetSliceIndex();
  cout << "Cursor Position : " << to_int(vCursor) << endl; // Ignore the to_int!
  cout << "Value at Cursor : " 
    << wrapGrey->GetVoxel(vCursor) << endl;

  /* The cursor position obtained above is given in image coordinates, where
   * Z is the slice number, Y is the row number and  X is the column number. 
   * In order to specify slice coordinates in patient (anatomical)
   * coordinates, we need to know the correct transformation, which we had \
   * specified using an 'RAI' code when loading the grey image. The 
   * transformation can be accessed through the IRISApplication object */
  ImageCoordinateGeometry geometry = irisData->GetImageGeometry();
  Vector3ui vCursorPatient = 
    geometry.GetImageToAnatomyTransform().TransformVoxelIndex(vCursor);
  cout << "Anatomical Posn.: " << to_int(vCursorPatient) << endl;

  /* The cursor position is the same for the grey image wrapper and the label
   * image wrapper. Hence, to set the cursor position, we must call the 
   * appropriate method in IRISImageData. We can also call the method
   * IRISApplication::SetCursorPosition for the same effect. */
  vCursor -= Vector3ui(1,0,0);
  irisData->SetCrosshairs(vCursor);

  /* ImageWrapper provides a way to extract orthogonal 2D slices form the
   * image. A slice is extracted by setting the crosshair position and calling
   * the GetDisplaySlice method. This returns a 2D image of type unsigned char
   * that can be displayed on the screen.
   *
   * The mapping between the intensities in the grey image (which are shorts) 
   * and intensities in the slice (which are bytes) can be changed by calling 
   * the method SetIntensityMapFunction() */
  GreyImageWrapper::DisplaySlicePointer imgSlice = wrapGrey->GetDisplaySlice(0);

  /* So far, we have looked at the grey image wrapper. The segmentation image 
   * wrapper is very similar, except that it produces slices that are RGB 
   * images.
   *
   * When we loaded the grey image, the segmentation image in IRISImageData was
   * automatically initialized to the same size and filled with voxels of 
   * label 0, which is the 'Clear' label in SNAP. To update the segmentation, 
   * we can directly modify the image wrapped by the LabelImageWrapper or
   * we can use the GetVoxelForUpdate() method: */
  irisData->GetSegmentation()->GetVoxelForUpdate(vCursor) = 2;
  LabelImageWrapper::DisplaySlicePointer imgRGBSlice = 
    irisData->GetSegmentation()->GetDisplaySlice(0);

  /* For validation purposes, we are going to write to disk the grey and 
   * segmentation slices that we've extracted in this Section */
  SaveImageToFile("greyslice01.png",imgSlice.GetPointer());
  SaveImageToFile("rgbslice01.png",imgRGBSlice.GetPointer());
  
  /* =======================================================================
   * Section 3. Automatic Segmentation Mode
   * ====================================================================== */
                                      
  /* To begin automatic segmentation, we need to switch on the automatic 
   * segmentation mode. In order to do that, we must specify the region of
   * interest that we want to pass on to this mode and whether or not we 
   * want to perform resampling on the region of interest. In this case we
   * will use the entire image as the region of interest and do no resampling */
  SNAPSegmentationROISettings roiSettings;
  roiSettings.SetROI(irisData->GetImageRegion());
  roiSettings.SetResampleFlag(false);
  
  /* We can now tell the IRISApplication to initialize the SNAPImageData object.
   * This method takes the ROI settings as the first parameter and an 
   * itk::Command object as the optional second parameter. The Command is used
   * as a callback during the resampling, and is provided primarify for GUIs 
   * that want to display a progress bar. */
  application.InitializeSNAPImageData(roiSettings);

  /* Once the SNAP data is initalized, we can instruct the IRISApplication to 
   * use it as the current image data, in other words, to switch to the auto
   * segmentation mode */
  application.SetCurrentImageDataToSNAP();

  /* The SNAPImageData class inherits the methods and attributes of the 
   * IRISImageData class and provides additional functionality related to 
   * level set image segmentation. In addition to the Grey and Segmentation
   * images (and image wrappers), the SNAPImageData class contains a Speed
   * image wrapper, a SnakeInitialization image wrapper and a Snake image
   * wrapper. All three of these image wrappers contain itk::Image objects
   * with pixel type float. In addition, the SNAPImageData class provides 
   * method for running the automatic segmetnation pipeline */
  SNAPImageData *snapData = application.GetSNAPImageData();

  /* Automatic segmentation begins with preprocessing. We must choose the type 
   * of preprocessing to apply and the parameters of the preprocessing. In this
   * tutorial we will use edge-based snakes and edge-detection preprocessing */
  EdgePreprocessingSettings edgeSettings = 
    EdgePreprocessingSettings::MakeDefaultSettings();
  edgeSettings.SetGaussianBlurScale(0.6);
  edgeSettings.SetRemappingSteepness(0.02);
  edgeSettings.SetRemappingExponent(2.0);
  
  /* The preprocessing is performed when we call the DoEdgePreprocessing method.
   * the first argument is an EdgePreprocessingSettings object and the second
   * optional argument is a Command that can be used to implement a progress bar*/
  snapData->DoEdgePreprocessing(edgeSettings);

  /* The result of the preprocessing is stored in the Speed image wrapper in
   * the SNAPImageData class. We can write out a slice of the preprocessed image */
  SpeedImageWrapper *wrapSpeed = snapData->GetSpeed();
  SaveImageToFile("speedslice00.mha",wrapSpeed->GetDisplaySlice(0));

  /* The next step after preprocessing is to initialize the segmentation with
   * bubbles. This is performed by passing an array of bubbles to the method
   * SNAPImageData::InitializeSegmentationPipeline. The bubbles given below
   * are located in the caudate nuclei of our image */
  std::vector<Bubble> bubbles(2);
  bubbles[0].center = Vector3i(50,29,26); bubbles[0].radius = 3;
  bubbles[1].center = Vector3i(50,57,34); bubbles[1].radius = 2;

  /* In addition to the bubble array, the method InitializeSegmentationPipeline
   * requires a set of level set segmentation parameters, which are stored in 
   * the SnakeParameters class */
  SnakeParameters parameters = SnakeParameters::GetDefaultEdgeParameters();

  /* In order for the snake to converge, we enable the advection force */
  parameters.SetAdvectionWeight(5.0);

  /* We can now initialize the segmentation pipeline. In addition to the 
   * snake parameters and the bubbles, we need to specify the color label to 
   * be used for the snake-based segmentation */
  snapData->InitializeSegmentation(
    parameters,bubbles,application.GetGlobalState()->GetDrawingColorLabel());

  /* Now the pipeline is initialized and ready to run. Run 250 iterations */
  snapData->RunSegmentation(250);

  /* We can also rewind the segmentation */
  snapData->RestartSegmentation();

  /* And we can change the parameters on the fly */
  snapData->RunSegmentation(100);
  parameters.SetAdvectionWeight(3.0);
  snapData->SetSegmentationParameters(parameters);
  snapData->RunSegmentation(200);

  /* Now the segmentation is done. The Snake image wrapper contains a floating
   * point image whose positive voxels correspond to the pixels outside of the
   * segmentation boundary and whose negative valued pixels are inside. In 
   * to get an actual snake surface, we can use the LevelSetMeshPipeline object,
   * which uses VTK to trace the zero-level-set of the Snake image */  
  LevelSetMeshPipeline meshPipeline;    
  meshPipeline.SetImage(snapData->GetSnake()->GetImage());
  // meshPipeline.SetImage(snapData->GetLevelSetImage());

  /* Pass the globally stored mesh options to the mesh pipeline */
  meshPipeline.SetMeshOptions(application.GetGlobalState()->GetMeshOptions());
  
  /* Create a vtkPolyData to store the resulting mesh, and run the pipeline */
  vtkPolyData *meshResult = vtkPolyData::New();  
  meshPipeline.ComputeMesh(meshResult);

  /* Export the mesh to VTK format */
  vtkPolyDataWriter *polyWriter = vtkPolyDataWriter::New();
  polyWriter->SetInput(meshResult);
  polyWriter->SetFileTypeToASCII();
  polyWriter->SetFileName("levelset.poly");
  polyWriter->Write();
  polyWriter->Delete();

  /* Great. We have now executed automatic segmentation and exported the leve set
   * as a mesh. What's left is to exit the automatic segmentation mode and to 
   * merge the segmentation results with the multi-label segmentation image 
   * stored in IRISImageData. The following method will do that for us. */
  application.UpdateIRISWithSnapImageData();
  application.SetCurrentImageDataToIRIS();

  /* Since we are done with the segmentation data, we should release resources
   * associated with it */
  application.ReleaseSNAPImageData();

  /* =======================================================================
   * Section 4. Additional Functionality
   * ====================================================================== */
                                      
  /* One of the features of SNAP it to be able to divide segmentations into two
   * labels using a cut-plane. This can be done programatically, using the
   * following code. In our example, this will divide the left and the right
   * caudates, giving them different labels */

  /* We will paint with label 9 over label 8 */
  application.GetGlobalState()->SetDrawingColorLabel(9);  
  application.GetGlobalState()->SetCoverageMode(PAINT_OVER_COLORS);
  application.GetGlobalState()->SetOverWriteColorLabel(8);  

  /* This actually performs the cut, given an normal vector and intercept that
   * define the cut plane */
  application.RelabelSegmentationWithCutPlane(Vector3d(1,0,0), 64);

  /* We can now save the segmentation result as a 3d image */
  SaveImageToFile("result.gipl",irisData->GetSegmentation()->GetImage());

  /* Thank you for reading this tutorial! */
  return 0;
}

// A global variable required for linkage
std::ostream &verbose = std::cout;
