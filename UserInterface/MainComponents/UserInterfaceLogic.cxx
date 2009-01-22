/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UserInterfaceLogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/22 23:14:10 $
  Version:   $Revision: 1.36 $
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
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "UserInterfaceLogic.h"

#include "GlobalState.h"
#include "GreyImageWrapper.h"
#include "RGBImageWrapper.h"
#include "EdgePreprocessingImageFilter.h"
#include "IntensityCurveUILogic.h"
#include "RGBOverlayUILogic.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "IRISVectorTypesToITKConversion.h"
#include "LabelEditorUILogic.h"
#include "SNAPImageData.h"
#include "SNAPLevelSetDriver.h"
#include "SNAPRegistryIO.h"
#include "SmoothBinaryThresholdImageFilter.h"
#include "SystemInterface.h"
#include "IRISSliceWindow.h"
#include "SNAPSliceWindow.h"
#include "Window3D.h"

// Additional UI component inludes
#include "AppearanceDialogUILogic.h"
#include "HelpViewerLogic.h"
#include "PreprocessingUILogic.h"
#include "SnakeParametersUILogic.h"
#include "GreyImageIOWizardLogic.h"
#include "RGBImageIOWizardLogic.h"
#include "PreprocessingImageIOWizardLogic.h"
#include "ResizeRegionDialogLogic.h"
#include "RestoreSettingsDialogLogic.h"
#include "ReorientImageUILogic.h"
#include "SegmentationImageIOWizardLogic.h"
#include "MeshIOWizardUILogic.h"
#include "SimpleFileDialogLogic.h"
#include "SliceWindowCoordinator.h"
#include "SNAPAppearanceSettings.h"
#include "FLTKWidgetActivationManager.h"

#include "FL/Fl_File_Chooser.H"

#include "itkImageIOBase.h"
#include <itksys/SystemTools.hxx>
// #include <strstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cctype>


// Global pointer to the 'current' UI object
UserInterfaceLogic* UserInterfaceLogic::m_GlobalUI = NULL;


// Disable some utterly annoying windows messages
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif

using namespace itk;
using namespace std;

#define COLORBAR_LABEL FL_FREE_LABELTYPE

void xyz_draw(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align) {  
  // We can't trust the label's color because it can be changed when the menu item
  // is selected. Instead, we encode the color in the label itself using octal notation
  unsigned int box_color;  
  sscanf(label->value,"%x",&box_color);
  fl_font(label->font, label->size);
  fl_color((Fl_Color)label->color);
  fl_draw(label->value + 9, x + h + label->size / 2, y, w-h, h, FL_ALIGN_LEFT);
  fl_draw_box(FL_BORDER_BOX, x, y, h, h, (Fl_Color) box_color);
}

void xyz_measure(const Fl_Label *label, int &w, int &h) 
{  
  fl_font(label->font, label->size);
  fl_measure(label->value + 9, w, h);
  w += h + label->size / 2; 
}


/**
 * \class GreyImageInfoCallback
 * \brief Adapter for the interface ImageInfoCallbackInterface, used to 
 * pass on registry information to the IO Wizard.
 */
class GreyImageInfoCallback : public ImageInfoCallbackInterface
{
public:
    virtual ~GreyImageInfoCallback() {}
  GreyImageInfoCallback(SystemInterface *system)
    { m_SystemInterface = system; }

  // This method finds the registry associated with a file
  bool FindRegistryAssociatedWithImage(const char *file, Registry &registry)
    { 
    m_Registry.Clear();
    if(!m_SystemInterface->FindRegistryAssociatedWithFile(file, m_Registry))
      return false;
    registry = m_Registry.Folder("Files.Grey");
    return true;
    }

  // This method updates the registry with values that the user specified
  void UpdateRegistryAssociatedWithImage(const char *file, Registry &folder)
    {
    // Create a registry into which to load the values
    Registry regNew;
    m_SystemInterface->FindRegistryAssociatedWithFile(file, regNew);

    // Update the corresponding folder
    regNew.Folder("Files.Grey").Update(folder);
      
    // Associate the settings
    m_SystemInterface->AssociateRegistryWithFile(file, regNew);
    }

private:
  SystemInterface *m_SystemInterface;
  Registry m_Registry;
};


class UserInterfaceLogicMemberObserver : 
  public FLTKWidgetActivationManager<UserInterfaceLogic::UIStateFlags>::Observer
{
public:
  // Member function type
  typedef void (UserInterfaceLogic::*TMemberFunctionPointer)(
    UserInterfaceLogic::UIStateFlags flag, bool value);

  // Constructor
  UserInterfaceLogicMemberObserver(
    UserInterfaceLogic *p, TMemberFunctionPointer member)
    {
    this->m_Pointer = p;
    this->m_Member = member;
    }

  // Callback
  void OnStateChange(UserInterfaceLogic::UIStateFlags flag, bool state)
    {
    if(m_Member && m_Pointer)
      {
      ((*m_Pointer).*(m_Member))(flag, state);
      }
    }
private:
  UserInterfaceLogic *m_Pointer;
  TMemberFunctionPointer m_Member;
};

void UserInterfaceLogic
::InitializeActivationFlags()
{
  unsigned int i;   
  
  // ---------------------------------------------------------
  //    Intialize activation object
  // ---------------------------------------------------------
  m_Activation = new FLTKWidgetActivationManager<UIStateFlags>;
  
  // ---------------------------------------------------------
  //    Configure Flag Relationships
  // ---------------------------------------------------------
  
  // Set the parent-child relationships between flags
  m_Activation->SetFlagImplies(UIF_SNAP_SPEED_AVAILABLE, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_MESH_CONTINUOUS_UPDATE, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_GRAY_LOADED, UIF_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_RGBBASE_LOADED, UIF_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_RGBBASE_LOADED, UIF_RGBANY_LOADED);
  m_Activation->SetFlagImplies(UIF_RGBOVL_LOADED, UIF_RGBANY_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_GRAY_LOADED, UIF_GRAY_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_BASEIMG_LOADED, UIF_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_BASEIMG_LOADED, UIF_IRIS_ACTIVE);
  m_Activation->SetFlagImplies(UIF_IRIS_WITH_GRAY_LOADED, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_MESH_DIRTY, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_MESH_ACTION_PENDING, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_IRIS_ROI_VALID, UIF_GRAY_LOADED);
  // m_Activation->SetFlagImplies(UIF_LINKED_ZOOM, UIF_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_SNAP_PREPROCESSING_ACTIVE, UIF_SNAP_ACTIVE);

  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_PREPROCESSING, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_BUBBLES, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_PAGE_SEGMENTATION, UIF_SNAP_ACTIVE);

  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_RUNNING, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_EDITABLE, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_EDITABLE, UIF_SNAP_SNAKE_RUNNING, true, false);
  m_Activation->SetFlagImplies(UIF_SNAP_SNAKE_INITIALIZED, UIF_SNAP_ACTIVE);
  m_Activation->SetFlagImplies(UIF_SNAP_PREPROCESSING_DONE, UIF_SNAP_ACTIVE);

  m_Activation->SetFlagImplies(UIF_UNDO_POSSIBLE, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_REDO_POSSIBLE, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->SetFlagImplies(UIF_UNSAVED_CHANGES, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Set up the exclusive relationships between flags
  m_Activation->SetFlagImplies( UIF_SNAP_ACTIVE, UIF_IRIS_ACTIVE, true, false );
  m_Activation->SetFlagImplies(UIF_SNAP_PREPROCESSING_DONE, 
    UIF_SNAP_PREPROCESSING_ACTIVE, true, false );  
  m_Activation->SetFlagsMutuallyExclusive( 
    UIF_SNAP_PAGE_SEGMENTATION, 
    UIF_SNAP_PAGE_BUBBLES, 
    UIF_SNAP_PAGE_PREPROCESSING );

  // ---------------------------------------------------------
  //    Add observers to flags
  // ---------------------------------------------------------
  m_Activation->AddObserver(
    new UserInterfaceLogicMemberObserver(
      this, &UserInterfaceLogic::OnUnsavedChangesStateChange),
    UIF_UNSAVED_CHANGES);

  
  // ---------------------------------------------------------
  //    Relate flags to widgets
  // ---------------------------------------------------------
  
  // Link widget activation to flags
  m_Activation->AddWidget(m_BtnAcceptPreprocessing, UIF_SNAP_PREPROCESSING_DONE);
  m_Activation->AddWidget(m_ChoiceSNAPView, UIF_SNAP_PREPROCESSING_DONE);
  m_Activation->AddWidget(m_BtnPreprocessedColorMap, UIF_SNAP_SPEED_AVAILABLE);
  m_Activation->AddWidget(m_BtnSNAPMeshUpdate, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnMeshUpdate, UIF_IRIS_MESH_DIRTY);
  m_Activation->AddWidget(m_BtnStartSnake, UIF_IRIS_ROI_VALID);
  m_Activation->AddWidget(m_InZoomLevel, UIF_LINKED_ZOOM);
  m_Activation->AddWidget(m_GrpLinkedZoomUnits, UIF_LINKED_ZOOM);
  m_Activation->AddWidget(m_ChkMultisessionZoom, UIF_LINKED_ZOOM);
  m_Activation->AddWidget(m_BtnAccept3D, UIF_IRIS_MESH_ACTION_PENDING);

  m_Activation->AddWidget(m_BtnAcceptSegmentation, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnRestartInitialization, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakePlay, UIF_SNAP_SNAKE_EDITABLE);
  m_Activation->AddWidget(m_BtnSnakeRewind, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakeStop, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddWidget(m_BtnSnakeStep, UIF_SNAP_SNAKE_INITIALIZED);

  m_Activation->AddWidget(m_BtnIRISUndo, UIF_UNDO_POSSIBLE);
  m_Activation->AddWidget(m_BtnIRISRedo, UIF_REDO_POSSIBLE);

  // Add more complex relationships
  m_Activation->AddCheckBox(m_ChkContinuousView3DUpdate, 
    UIF_SNAP_SNAKE_INITIALIZED, false, false);

  // Activate slice-related widgets indexed by dimension
  for(i = 0; i < 3; i++)
    {
    m_Activation->AddWidget(m_InSliceSlider[i], UIF_BASEIMG_LOADED);
    m_Activation->AddWidget(m_OutSliceIndex[i], UIF_BASEIMG_LOADED);
    }

  // Activate the widgets that have four copies
  for(i = 0; i < 4; i++)
    {
    m_Activation->AddWidget(m_BtnSaveAsPNG[i], UIF_BASEIMG_LOADED);
    m_Activation->AddWidget(m_BtnResetView[i], UIF_BASEIMG_LOADED);
    m_Activation->AddWidget(m_BtnPanelZoom[i], UIF_BASEIMG_LOADED);
    }

  // Link menu items to flags
  m_Activation->AddMenuItem(m_MenuLoadGrey, UIF_IRIS_ACTIVE);
  m_Activation->AddMenuItem(m_MenuLoadRGB, UIF_IRIS_ACTIVE);
  m_Activation->AddMenuItem(m_MenuSaveGrey, UIF_IRIS_WITH_GRAY_LOADED); 
  m_Activation->AddMenuItem(m_MenuLoadRGBOverlay, UIF_IRIS_WITH_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuLoadSegmentation, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuNewSegmentation, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveGreyROI, UIF_SNAP_ACTIVE);
  m_Activation->AddMenuItem(m_MenuSaveSegmentation, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveSegmentationMesh, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveLabels, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuLoadLabels, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveVoxelCounts, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveScreenshot, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuSaveScreenshotSeries, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuIntensityCurve, UIF_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuRGBOverlayOptions, UIF_RGBANY_LOADED);
  m_Activation->AddMenuItem(m_MenuExportSlice, UIF_GRAY_LOADED);
  m_Activation->AddMenuItem(m_MenuSavePreprocessed, UIF_SNAP_PREPROCESSING_DONE);
  m_Activation->AddMenuItem(m_MenuSaveLevelSet, UIF_SNAP_SNAKE_INITIALIZED);
  m_Activation->AddMenuItem(m_MenuLoadPreprocessed, UIF_SNAP_PAGE_PREPROCESSING);
  m_Activation->AddMenuItem(m_MenuLoadAdvection, UIF_SNAP_PAGE_PREPROCESSING);
  m_Activation->AddMenuItem(m_MenuImageInfo, UIF_IRIS_WITH_BASEIMG_LOADED);
  m_Activation->AddMenuItem(m_MenuReorientImage, UIF_IRIS_WITH_BASEIMG_LOADED);
  

  for(i = 0; i < 5; i++)
    m_Activation->AddMenuItem(m_MenuLoadPreviousFirst + i, UIF_IRIS_ACTIVE);


}

UserInterfaceLogic
::UserInterfaceLogic(IRISApplication *iris)
: UserInterface()
{
  // Initialize the menu lists to NULL
  m_MenuDrawingLabels = NULL;
  m_MenuDrawOverLabels = NULL;

  // TODO: move this!
  Fl::set_labeltype(COLORBAR_LABEL, xyz_draw, xyz_measure);

  // Store the pointers to application and system high level objects
  m_Driver = iris;
  m_SystemInterface = iris->GetSystemInterface();

  // This is just done for shorthand
  m_GlobalState = iris->GetGlobalState();

  // Load the appearance settings from the system interface
  m_AppearanceSettings = new SNAPAppearanceSettings();
  Registry &regAppearance = 
    m_SystemInterface->Folder("UserInterface.AppearanceSettings");
  m_AppearanceSettings->LoadFromRegistry(regAppearance);

  // Instantiate the IO wizards
  m_WizGreyIO = new GreyImageIOWizardLogic; 
  m_WizRGBIO = new RGBImageIOWizardLogic; 
  m_WizRGBOverlayIO = new RestrictedRGBImageIOWizardLogic; 
  m_WizSegmentationIO = new SegmentationImageIOWizardLogic;
  m_WizPreprocessingIO = new PreprocessingImageIOWizardLogic;
  m_WizLevelSetIO = new PreprocessingImageIOWizardLogic;
  m_WizMeshExport = new MeshIOWizardUILogic;
  
  // Initialize the Image IO wizards
  m_WizGreyIO->MakeWindow();
  m_WizRGBIO->MakeWindow();
  m_WizRGBOverlayIO->MakeWindow();
  m_WizSegmentationIO->MakeWindow();
  m_WizPreprocessingIO->MakeWindow();
  m_WizLevelSetIO->MakeWindow();
  m_WizMeshExport->MakeWindow();

  // Provide the registry callback for the greyscale image wizard
  m_GreyCallbackInterface = new GreyImageInfoCallback(m_SystemInterface);
  m_WizGreyIO->SetImageInfoCallback(m_GreyCallbackInterface);
  
  // Instantiate other windows
  m_IntensityCurveUI = new IntensityCurveUILogic;
  m_IntensityCurveUI->MakeWindow();

  // Instantiate RGB Overlay options window
  m_RGBOverlayUI = new RGBOverlayUILogic;
  m_RGBOverlayUI->MakeWindow();
  SimpleCommandType::Pointer cmdRGBOverlayUI = SimpleCommandType::New();
  cmdRGBOverlayUI->SetCallbackFunction(this, &UserInterfaceLogic::OnRGBOverlayOptionsUpdate);
  m_RGBOverlayUI->GetEventSystem()->AddObserver(
    RGBOverlayUILogic::OpacityUpdateEvent(), cmdRGBOverlayUI);
  
  // Create the label editor window
  m_LabelEditorUI = new LabelEditorUILogic();
  m_LabelEditorUI->MakeWindow();
  m_LabelEditorUI->Register(this);
  
  // Add a listener for the events generated by the curve editor
  SimpleCommandType::TMemberFunctionPointer ptrMember = 
    & UserInterfaceLogic::OnIntensityCurveUpdate;

  SimpleCommandType::Pointer cmd = SimpleCommandType::New();
  cmd->SetCallbackFunction(this,ptrMember);
  m_IntensityCurveUI->GetEventSystem()->AddObserver(
    IntensityCurveUILogic::CurveUpdateEvent(),cmd);
  
  // Initialize the progress command
  m_ProgressCommand = ProgressCommandType::New();
  m_ProgressCommand->SetCallbackFunction(
    this,&UserInterfaceLogic::OnITKProgressEvent);

  // Initialize the preprocessing windows
  m_PreprocessingUI = new PreprocessingUILogic;
  m_PreprocessingUI->MakeWindow();
  m_PreprocessingUI->Register(this);

  // Initialize the snake parameter window
  m_SnakeParametersUI = new SnakeParametersUILogic;
  m_SnakeParametersUI->MakeWindow();
  m_SnakeParametersUI->Register(this);

  // Initialize the restore settings dialog
  m_DlgRestoreSettings = new RestoreSettingsDialogLogic;
  m_DlgRestoreSettings->MakeWindow();

  // Initialize the resample dialog
  m_DlgResampleRegion = new ResizeRegionDialogLogic;
  m_DlgResampleRegion->MakeWindow();

  // Initialize the appearance dialog
  m_DlgAppearance = new AppearanceDialogUILogic;
  m_DlgAppearance->MakeWindow();
  m_DlgAppearance->Register(this);

  // Initialize the reorientation dialog
  m_DlgReorientImage = new ReorientImageUILogic;
  m_DlgReorientImage->MakeWindow();
  m_DlgReorientImage->Register(this);

  // Create the window managers for SNAP and IRIS. Start in IRIS mode
  for(int i=0; i<3; i++)
    {    
    m_IRISWindowManager2D[i] = new IRISSliceWindow(i, this, m_SliceWindow[i]);
    m_SNAPWindowManager2D[i] = new SNAPSliceWindow(i, this, m_SliceWindow[i]);
    m_SliceWindow[i]->PushInteractionMode(m_IRISWindowManager2D[i]);
    }

  // Create the 3D Window managers for SNAP and IRIS
  m_IRISWindowManager3D = new Window3D(this, m_RenderWindow);
  m_SNAPWindowManager3D = new Window3D(this, m_RenderWindow);
  m_RenderWindow->PushInteractionMode(m_IRISWindowManager3D);

  // Initialize the slice window coordinator object
  m_SliceCoordinator = new SliceWindowCoordinator();
  
  // Group the three windows inside the window coordinator
  m_SliceCoordinator->RegisterWindows(
    reinterpret_cast<GenericSliceWindow **>(m_IRISWindowManager2D));    

  // Create a callback command for the snake loop
  m_PostSnakeCommand = SimpleCommandType::New();

  // Initialize the Help UI
  m_HelpUI = new HelpViewerLogic;
  m_HelpUI->MakeWindow();
  m_HelpUI->SetContentsLink(
    m_SystemInterface->GetFileInRootDirectory("HTMLHelp/Tutorial.html").c_str());

  //  Initialize the label IO dialog
  m_DlgLabelsIO = new SimpleFileDialogLogic();
  m_DlgLabelsIO->MakeWindow();
  m_DlgLabelsIO->SetFileBoxTitle("Label description file:");
  m_DlgLabelsIO->SetPattern("All Label Files (*.{label,lbl,lab,txt})");
  m_DlgLabelsIO->SetLoadCallback(this,&UserInterfaceLogic::OnLoadLabelsAction);
  m_DlgLabelsIO->SetSaveCallback(this,&UserInterfaceLogic::OnSaveLabelsAction);

  /** Write voxels dialog */
  m_DlgVoxelCountsIO = new SimpleFileDialogLogic();
  m_DlgVoxelCountsIO->MakeWindow();
  m_DlgVoxelCountsIO->SetFileBoxTitle("Voxel count file:");
  m_DlgVoxelCountsIO->SetPattern("Text files (*.{txt})");
  m_DlgVoxelCountsIO->SetSaveCallback(
    this,&UserInterfaceLogic::OnWriteVoxelCountsAction);

  // Set the welcome page to display
  m_WizControlPane->value(m_GrpWelcomePage);

  InitializeActivationFlags();
  InitializeUI();

  // Update the recent files menu
  GenerateRecentFilesMenu();

  // Enter the IRIS-ACTiVE state
  m_Activation->UpdateFlag(UIF_IRIS_ACTIVE, true);

  // Opacity toggle value set to default
  m_OpacityToggleValue = 128;

}

UserInterfaceLogic
::~UserInterfaceLogic() 
{
  // Delete the menus
  DeleteColorLabelMenu(m_MenuDrawingLabels);
  DeleteColorLabelMenu(m_MenuDrawOverLabels);

  // Delete the IO wizards
  delete m_WizGreyIO;
  delete m_WizRGBIO;
  delete m_WizRGBOverlayIO;
  delete m_WizSegmentationIO;
  delete m_WizPreprocessingIO;
  delete m_WizLevelSetIO;
  delete m_WizMeshExport;

  // Delete the IO wizard registry adaptor
  delete m_GreyCallbackInterface;

  // Other IO dialogs
  delete m_DlgLabelsIO;
  delete m_DlgVoxelCountsIO;

  // Delete the UI's
  delete m_IntensityCurveUI;
  delete m_SnakeParametersUI;
  delete m_PreprocessingUI;
  delete m_DlgRestoreSettings;
  delete m_DlgResampleRegion;
  delete m_DlgAppearance;
  delete m_DlgReorientImage;
  delete m_LabelEditorUI;
  delete m_RGBOverlayUI;

  // Delete the window managers
  for(int i = 0; i < 3; i++)
    {
    delete m_IRISWindowManager2D[i];
    delete m_SNAPWindowManager2D[i];
    }
  delete m_IRISWindowManager3D;
  delete m_SNAPWindowManager3D;

  // Delete the window coordinator
  delete m_SliceCoordinator;

  // Delete the appearance settings
  delete m_AppearanceSettings;

  // Delete the activation manager
  delete m_Activation;
}

void 
UserInterfaceLogic
::OnResetROIAction()
{
  if (!m_GreyFileLoaded) return;

  // Get the grey image's region
  GlobalState::RegionType roi = 
    m_Driver->GetIRISImageData()->GetImageRegion();

  // The region can not be empty!
  assert(roi.GetNumberOfPixels() > 0);

  // Set the Region of interest
  m_GlobalState->SetSegmentationROI(roi);
  m_GlobalState->SetIsValidROI(true);
  
  // Update the UI
  this->RedrawWindows();
}

//--------------------------------------------
//
//
// SEGMENT 3D BUTTON CALLBACK
//
//
//--------------------------------------------

unsigned int 
UserInterfaceLogic
::GetImageAxisForDisplayWindow(unsigned int window)
{
  return m_Driver->GetCurrentImageData()->
    GetImageGeometry().GetDisplayToImageTransform(window).
    GetCoordinateIndexZeroBased(2);
}

void 
UserInterfaceLogic
::OnSnakeStartAction()
{
  uchar index = m_GlobalState->GetDrawingColorLabel();

  if (0 == index) 
    {
    fl_alert("Cannot start snake segmentation with clear color");
    return;
    }

  if (!m_Driver->GetColorLabelTable()->GetColorLabel(index).IsVisible()) 
    {
    fl_alert("Current label must be visible to start snake segmentation");
    return;
    }

  // Get the region of interest
  SNAPSegmentationROISettings roi = m_GlobalState->GetSegmentationROISettings();

  // The voxel size for the resampled region
  Vector3d voxelSizeSrc(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage()->GetSpacing().GetDataPointer());

  // Check if the user wants to resample the image
  if(m_ChkResampleRegion->value())
    {
    // Show the resampling dialog, updating the ROI object
    m_DlgResampleRegion->DisplayDialog(voxelSizeSrc.data_block(), roi);
    }
  else
    {
    roi.SetVoxelScale(Vector3d(1.0));
    roi.SetResampleFlag(false);
    }

  // Update the segmentation ROI
  m_GlobalState->SetSegmentationROISettings(roi);

  // The region can not be empty
  assert(roi.GetROI().GetNumberOfPixels() > 0);

  // Set the current application image mode to SNAP data
  m_Driver->InitializeSNAPImageData(roi,m_ProgressCommand);
  m_Driver->SetCurrentImageDataToSNAP();

  // Inform the global state that we're in sNAP
  m_GlobalState->SetSNAPActive(true);

  // Set bubble radius range according to volume dimensions (world dimensions)
  Vector3ui size = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3d voxdims = m_Driver->GetSNAPImageData()->GetImageSpacing();
  double mindim = 
    vector_multiply_mixed<double,unsigned int,3>(voxdims, size).min_value();

  // The largest value of the bubble radius is mindim / 2
  double xBubbleMax = 0.5 * mindim ;

  // The unit step should be equal or smaller than the smallest voxel edge length
  // divided by two, and should be a power of 10. Since FLTK accepts rational step
  // size, we compute it as a ratio two numbers
  double xMinVoxelEdge = 0.5 * voxdims.min_value();
  int xBubbleStepA = 1, xBubbleStepB = 1;
  int xLogVoxelEdge = (int) floor(log10(xMinVoxelEdge));
  if(xLogVoxelEdge > 0)
    xBubbleStepA = (int)(0.5 + pow(10.0, xLogVoxelEdge));
  else if(xLogVoxelEdge < 0)
    xBubbleStepB = (int)(0.5 + pow(10.0, -xLogVoxelEdge));
  
  // It is likely however that 0.1 is not an appropriate step size when min 
  // voxel size is 0.99, so we try 0.5 and 0.2 as candidates
  if(xBubbleStepA * 5.0 / xBubbleStepB <= xMinVoxelEdge)
    xBubbleStepA *= 5;
  else if(xBubbleStepA * 2.0 / xBubbleStepB <= xMinVoxelEdge)
    xBubbleStepA *= 2;

  // Set the bubble min value
  double xBubbleStep = xBubbleStepA * 1.0 / xBubbleStepB;
  double xBubbleMin = xBubbleStep;

  // Set the default value so that it falls on the step boundary
  double xBubbleDefault = floor(0.25 * xBubbleMax / xBubbleStep) * xBubbleStep;

  // Set the value for the radius slider
  m_InBubbleRadius->range(xBubbleMin, xBubbleMax);
  m_InBubbleRadius->step(xBubbleStepA, xBubbleStepB);
  m_InBubbleRadius->value(xBubbleDefault);
  m_InBubbleRadius->redraw();

  // Use the current SnakeParameters to determine which type of snake to use
  const SnakeParameters &parameters = m_GlobalState->GetSnakeParameters();
  if(parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
    {
    m_RadSnakeEdge->set();
    m_RadSnakeInOut->clear();
    m_GlobalState->SetSnakeMode(EDGE_SNAKE);
    OnEdgeSnakeSelect();
    }
  else
    {
    m_RadSnakeInOut->set();
    m_RadSnakeEdge->clear();
    m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);
    OnInOutSnakeSelect();
    }
  
  // The edge preprocessing settings pass through unchanged
  // Get the current thresholding properties
  ThresholdSettings threshSettings = m_GlobalState->GetThresholdSettings();

  // We want to keep the current preprocessing settings, but we have to be
  // careful that they are in range of image intensity
  GreyType iMax =  m_Driver->GetCurrentImageData()->GetGrey()->GetImageMax();
  GreyType iMin =  m_Driver->GetCurrentImageData()->GetGrey()->GetImageMin();
  if(threshSettings.GetUpperThreshold() > iMax)
    threshSettings.SetUpperThreshold(iMax);
  if(threshSettings.GetLowerThreshold() < iMin)
    threshSettings.SetLowerThreshold(iMin);
  m_GlobalState->SetThresholdSettings(threshSettings);


  // This method basically sends the current button state from IRIS to SNAP
  SyncSnakeToIRIS();

  // Initialize GUI widgets 
  // TODO: WTF is this?
  m_ChoiceSNAPView->value(m_MenuSNAPViewOriginal);
  // adioSNAPViewPreprocessed->value(0);
  //RadioSNAPViewOriginal->value(1);
  m_BtnAcceptInitialization->show();
  m_BtnRestartInitialization->hide();

  m_GlobalState->SetShowSpeed(false);

  m_BrsActiveBubbles->clear();

  uchar rgb[3];
  m_Driver->GetColorLabelTable()->GetColorLabel(index).GetRGBVector(rgb);
  m_GrpSNAPCurrentColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));
  m_GrpSNAPCurrentColor->redraw();

  m_SnakeStepSize = 1;
  m_InStepSize->value(0);

  // reset Mesh in IRIS window
  m_IRISWindowManager3D->ClearScreen();

  // Hide the label editor since it's open
  m_LabelEditorUI->OnCloseAction();

  // show the snake window, hide the IRIS window
  ShowSNAP();

  // We are going to preserve the cursor position if it was in the ROI
  Vector3ui newCursor, oldCursor = m_Driver->GetCursorPosition();

  // Image geometry has changed. This method also resets the cursor 
  // position, which is something we don't want.
  OnImageGeometryUpdate();

  // So we reset the cursor position manually here if the cursor was in the ROI
  if(roi.TransformImageVoxelToROIVoxel(oldCursor, newCursor))
    {
    m_Driver->SetCursorPosition(newCursor);
    this->OnCrosshairPositionUpdate();
    }
}

//--------------------------------------------
//
//
// PREPROCESSING
//
//
//--------------------------------------------
void 
UserInterfaceLogic
::OnPreprocessAction()
{
  // Disable the 'Next' button on the preprocessing page
  m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_ACTIVE, true);

  // Display the right window
  if(m_GlobalState->GetSnakeMode() == EDGE_SNAKE)
    {
    m_PreprocessingUI->DisplayEdgeWindow();
    }
  else
    {
    m_PreprocessingUI->DisplayInOutWindow();
    }
}

void 
UserInterfaceLogic
::OnPreprocessClose()
{
  // Check if the preprocessing image has been computed
  if(m_GlobalState->GetSpeedValid())
    m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, true);
  else
    {
    m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_ACTIVE, false);    
    m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);
    }
}

void 
UserInterfaceLogic
::OnAcceptPreprocessingAction()
{  
  SetActiveSegmentationPipelinePage(1);
}

void 
UserInterfaceLogic
::OnPreprocessedColorMapAction()
{
  // Set up mapping from color maps to menu items (TODO: this is hacky)
  static std::map<ColorMapPreset, int> menuMap;
  if(menuMap.size() == 0)
    {
    // Edge items
    menuMap[COLORMAP_BLACK_BLACK_WHITE] = 0;
    menuMap[COLORMAP_BLACK_YELLOW_WHITE] = 1;

    // Region items
    menuMap[COLORMAP_BLACK_GRAY_WHITE] = 0;
    menuMap[COLORMAP_BLUE_BLACK_WHITE] = 1;    
    menuMap[COLORMAP_BLUE_WHITE_RED] = 2;
    }

  // Select current color maps in the menus
  m_ChoiceColorMap[0]->value(menuMap[m_GlobalState->GetSpeedColorMapInRegionMode()]);
  m_ChoiceColorMap[1]->value(menuMap[m_GlobalState->GetSpeedColorMapInEdgeMode()]);

  // Set the ranges for the color map boxes
  m_BoxColorMap[0]->SetRange(-1.0, 1.0);
  m_BoxColorMap[1]->SetRange(0.0, 1.0);

  // Show the color map box and window
  m_WinColorMap->show();

  // Update the color map in the speed images
  UpdateSpeedColorMap();
}

void 
UserInterfaceLogic
::OnColorMapCloseAction()
{
  m_WinColorMap->hide();
}

void 
UserInterfaceLogic
::OnColorMapSelectAction()
{
  // Mapping from edge menu items to color maps
  static const ColorMapPreset edgeMenuMap[] = 
    { COLORMAP_BLACK_BLACK_WHITE, COLORMAP_BLACK_YELLOW_WHITE };
  static const ColorMapPreset regionMenuMap[] = 
    { COLORMAP_BLACK_GRAY_WHITE, COLORMAP_BLUE_BLACK_WHITE, 
      COLORMAP_BLUE_WHITE_RED };
  
  // Select the appropriate page in the color map window
  ColorMapPreset xPreset =  
    (m_GlobalState->GetSnakeMode() == EDGE_SNAKE) 
    ? edgeMenuMap[m_ChoiceColorMap[1]->value()] 
    : regionMenuMap[m_ChoiceColorMap[0]->value()] ;

  // Set the current color map
  m_GlobalState->SetSpeedColorMap(xPreset);

  // Update the display
  UpdateSpeedColorMap();
}


void 
UserInterfaceLogic
::UpdateSpeedColorMap()
{
  // Get the index of the color map box to update
  int iBox = (m_GlobalState->GetSnakeMode() == EDGE_SNAKE) ? 1 : 0;

  // Select the appropriate page in the color map window
  m_WizColorMap->value(m_GrpColorMapPage[iBox]);

  // Apply the color map to the preview window
  m_BoxColorMap[iBox]->SetColorMap(
    SpeedColorMap::GetPresetColorMap(
      m_GlobalState->GetSpeedColorMap()));
  m_BoxColorMap[iBox]->redraw();

  // Apply the color map to the speed wrapper
  if(m_Driver->GetSNAPImageData()->IsSpeedLoaded())
    m_Driver->GetSNAPImageData()->GetSpeed()->
      SetColorMap(SpeedColorMap::GetPresetColorMap(
        m_GlobalState->GetSpeedColorMap()));

  // Also, apply the color map to the SNAP preview window
  if(m_Driver->GetSNAPImageData()->IsSpeedLoaded())
    m_SnakeParametersUI->OnSpeedColorMapUpdate();

  // If the color map window is showing, show the color boxes
  if(m_WinColorMap->shown())
    {
    m_BoxColorMap[1 - iBox]->hide();
    m_BoxColorMap[iBox]->show();
    }

  // Redraw the windows
  RedrawWindows();
}


void 
UserInterfaceLogic
::OnPreprocessingPreviewStatusUpdate(bool flagPreview)
{
  // Enable the preprocessing widgets and color map button
  m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, flagPreview);
  
  // Make sure that the preview mode is used
  m_ChoiceSNAPView->value(flagPreview ? 
    m_MenuSNAPViewPreprocessed : m_MenuSNAPViewOriginal);

  // Set whether speed is shown
  m_GlobalState->SetShowSpeed(flagPreview);

  // Redraw the windows
  RedrawWindows();
}

//--------------------------------------------
//
//
// SWITCH BETWEEN GREY/PREPROC DATA
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::OnSNAPViewOriginalSelect()
{
  m_GlobalState->SetShowSpeed(false);
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnViewPreprocessedSelect()
{
  if (!m_GlobalState->GetSpeedValid())
    return;
  m_GlobalState->SetShowSpeed(true);
  RedrawWindows();
}

//--------------------------------------------
//
//
// BUBBLES
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::UpdateBubbleUI()
{
  // Fill the array of values
  m_BrsActiveBubbles->clear();
  GlobalState::BubbleArray ba = m_GlobalState->GetBubbleArray();
  for(unsigned int i = 0; i < ba.size(); i++)
    {
    std::ostringstream oss;
    oss << "C=" << ba[i].center << "; ";
    oss << "R=" << std::setprecision(3) << ba[i].radius;
    m_BrsActiveBubbles->add(oss.str().c_str());
    }
  
  // Get the active bubble
  int ibub = m_GlobalState->GetActiveBubble();
  
  // The browser uses 1-based indexing, so we add 1
  m_BrsActiveBubbles->value(ibub + 1);

  // Set the radius slider
  if(ibub >= 0)
    m_InBubbleRadius->value(ba[ibub].radius);

  // Redraw the browser
  m_BrsActiveBubbles->redraw();  

  // Redraw the windows as well
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnAddBubbleAction()
{
  // Create a new bubble
  Bubble bub;
  bub.center = to_int(m_Driver->GetCursorPosition());
  bub.radius = m_InBubbleRadius->value();

  // Add the bubble to the global state
  GlobalState::BubbleArray ba = m_GlobalState->GetBubbleArray();
  ba.push_back(bub);
  m_GlobalState->SetBubbleArray(ba);

  // Set the bubble's position
  m_GlobalState->SetActiveBubble(ba.size() - 1);

  // Update the bubble list in the GUI
  UpdateBubbleUI();
}

void 
UserInterfaceLogic
::OnRemoveBubbleAction()
{
  int ibub = m_GlobalState->GetActiveBubble();
  if(ibub >= 0) 
    {
    // Remove the bubble from the global state
    GlobalState::BubbleArray ba = m_GlobalState->GetBubbleArray();
    ba.erase(ba.begin() + ibub);
    m_GlobalState->SetBubbleArray(ba);

    // Update the active bubble
    if(ibub == (int) ba.size())
      m_GlobalState->SetActiveBubble(ibub - 1);

    // Update the bubble list in the GUI
    UpdateBubbleUI();
    } 
  else
    {
    fl_alert("To remove a bubble, first select a bubble in the list.");
    }
}

void 
UserInterfaceLogic
::OnActiveBubblesChange()
{
  // Set the active bubble in the global state
  m_GlobalState->SetActiveBubble(m_BrsActiveBubbles->value() - 1);

  // Update the user interface
  UpdateBubbleUI();
}

void 
UserInterfaceLogic
::OnBubbleRadiusChange()
{
  int ibub = m_GlobalState->GetActiveBubble();
  if(ibub >= 0)
    {
    // Update the bubble in the global state
    GlobalState::BubbleArray ba = m_GlobalState->GetBubbleArray();
    ba[ibub].radius = m_InBubbleRadius->value();
    m_GlobalState->SetBubbleArray(ba);

    // Update the bubble list in the GUI
    UpdateBubbleUI();
    }
}

//--------------------------------------------
//
//
// SNAKE TYPE RADIO BUTTONS
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::OnInOutSnakeSelect()
{
  m_RadSnakeInOut->set();
  m_RadSnakeEdge->clear();
  m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);

  //Nathan Moon
  if (m_GlobalState->GetSpeedValid()) 
    {
    //make sure they want to do this
    //fl_ask is deprecated if (0 == fl_ask("Preprocessed data will be lost!  Continue?"))
    if (0 == fl_choice("Preprocessed data will be lost!  Continue?","No","Yes",NULL))
      {
      m_RadSnakeInOut->clear();
      m_RadSnakeEdge->set();
      m_GlobalState->SetSnakeMode(EDGE_SNAKE);
      return;
      }
    }

  // Set parameters to default values
  m_GlobalState->SetSnakeParameters(
    SnakeParameters::GetDefaultInOutParameters());

  m_Driver->GetSNAPImageData()->ClearSpeed();

  m_GlobalState->SetSpeedValid(false);

  // Update widget state
  m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);
  m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, false);

  m_PreprocessingUI->HidePreprocessingWindows();

  // Set the SNAP view to the grayscale mode
  m_ChoiceSNAPView->value(m_MenuSNAPViewOriginal);
  OnSNAPViewOriginalSelect();

  // Update the speed color map
  UpdateSpeedColorMap();
  
  // m_RadioSNAPViewOriginal->setonly();
}

void 
UserInterfaceLogic
::OnEdgeSnakeSelect()
{
  m_RadSnakeEdge->set();
  m_RadSnakeInOut->clear();
  m_GlobalState->SetSnakeMode(EDGE_SNAKE);

  //Nathan Moon
  if (m_GlobalState->GetSpeedValid()) 
    {
    //make sure they want to do this
    //fl_ask is deprecated if (0 == fl_ask("Preprocessed data will be lost!  Continue?")) 
    if (0 == fl_choice("Preprocessed data will be lost!  Continue?","No","Yes",NULL))
      {
      m_RadSnakeInOut->set();
      m_RadSnakeEdge->clear();
      m_GlobalState->SetSnakeMode(IN_OUT_SNAKE);
      return;
      }
    }

  // Set parameters to default values
  m_GlobalState->SetSnakeParameters(
    SnakeParameters::GetDefaultEdgeParameters());

  if (m_Driver->GetSNAPImageData()) m_Driver->GetSNAPImageData()->ClearSpeed();
  m_GlobalState->SetSpeedValid(false);
  
  // Update widget state
  m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);
  m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, false);
  
  m_PreprocessingUI->HidePreprocessingWindows();
  
  // Set the SNAP view to the grayscale mode
  m_ChoiceSNAPView->value(m_MenuSNAPViewOriginal);
  OnSNAPViewOriginalSelect();
  
  // Update the speed color map
  UpdateSpeedColorMap();
  
  // m_RadioSNAPViewOriginal->setonly();
}

/*
 * This method is called when the user has finished adding bubbles
 */
void 
UserInterfaceLogic
::OnAcceptInitializationAction()
{
  // Get bubbles, turn them into segmentation
  vector<Bubble> bubbles = m_GlobalState->GetBubbleArray();

  // Shorthand
  SNAPImageData *snapData = m_Driver->GetSNAPImageData();

  // Put on a wait cursor
  // TODO: Progress bar is needed here
  m_WinMain->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Merge bubbles with the segmentation image and initialize the snake
  bool rc = snapData->InitializeSegmentation(
    m_GlobalState->GetSnakeParameters(), bubbles, 
    m_GlobalState->GetDrawingColorLabel());

  // Restore the cursor
  m_WinMain->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  
  // Check if we need to bail out
  if (!rc) 
    {    
    // There were no voxels selected in the end
    fl_alert("Can not proceed without an initialization\n"
             "Please place a bubble into the image!");
    return;        
    } 

  m_BtnAcceptInitialization->hide();
  m_BtnRestartInitialization->show();

  // Set the UI for the segmentation state
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_EDITABLE, true);
  
  m_SNAPWindowManager3D->ClearScreen(); // reset Mesh object in Window3D_s
  m_SNAPWindowManager3D->ResetView();   // reset cursor

  // Flip to the next page in the wizard
  SetActiveSegmentationPipelinePage(2);

  m_GlobalState->SetSnakeActive(true);

  OnSnakeUpdate();
}


void 
UserInterfaceLogic
::SetActiveSegmentationPipelinePage(unsigned int page)
{
  switch(page)
    {
    case 0 : 
      m_WizSegmentationPipeline->value(m_GrpSNAPStepPreprocess);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_PREPROCESSING, true);
      break;

    case 1 :
      m_WizSegmentationPipeline->value(m_GrpSNAPStepInitialize);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_BUBBLES, true);
      break;

    case 2 :
      m_WizSegmentationPipeline->value(m_GrpSNAPStepSegment);
      m_Activation->UpdateFlag(UIF_SNAP_PAGE_SEGMENTATION, true);
      break;
    }
}

void 
UserInterfaceLogic
::OnRestartInitializationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // If the segmentation pipeline is active, deactivate it
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Tell the UI to activate related widgets
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_INITIALIZED, false);

  m_GlobalState->SetSnakeActive(false);
  m_BtnRestartInitialization->hide();
  m_BtnAcceptInitialization->show();

  m_SNAPWindowManager3D->ClearScreen(); // reset Mesh object in Window3D_s
  m_SNAPWindowManager3D->ResetView();   // reset cursor
  RedrawWindows();

  // Flip to the second page
  SetActiveSegmentationPipelinePage(1);
}

void
UserInterfaceLogic
::OnRestartPreprocessingAction()
{
  // Reset the active bubble (if there are any)
  m_GlobalState->SetActiveBubble(
    m_GlobalState->GetBubbleArray().size() > 0 ? 0 : -1);

  // Flip to the first page
  SetActiveSegmentationPipelinePage(0);

  // Repaint the screen and the bubbles
  UpdateBubbleUI();
}

void 
UserInterfaceLogic
::OnSnakeParametersAction()
{
  // Get the current parameters from the system
  SnakeParameters pGlobal = m_GlobalState->GetSnakeParameters();
  
  // Send current parameter to the snake parameter setting UI
  m_SnakeParametersUI->SetParameters(pGlobal);

  // Chech whether we need to warn user about changing the solver in the 
  // process of evolution
  m_SnakeParametersUI->SetWarnOnSolverUpdate(
    m_Driver->GetSNAPImageData()->GetElapsedSegmentationIterations() > 0);
  
  // Show the snake parameters window
  CenterChildWindowInParentWindow(m_SnakeParametersUI->GetWindow(),m_WinMain);
  m_SnakeParametersUI->DisplayWindow();
  
  // Wait until the window has been closed
  while(m_SnakeParametersUI->GetWindow()->visible())
    Fl::wait();
    
  // Have the parameters been accepted by the user?
  if(m_SnakeParametersUI->GetUserAccepted())
    {
    // Get the new parameters
    SnakeParameters pNew = m_SnakeParametersUI->GetParameters();
    
    // Have the parameters changed?
    if(!(pGlobal == pNew))
      {
      // Update the system's parameters with new values
      m_GlobalState->SetSnakeParameters(pNew);

      // Update the running snake
      if (m_Driver->GetSNAPImageData()->IsSegmentationActive()) 
        {
        m_Driver->GetSNAPImageData()->SetSegmentationParameters(pNew);
        }
      }
    }
}

void 
UserInterfaceLogic
::OnSnakeRewindAction()
{
  // Stop the snake if it's running
  OnSnakeStopAction();

  // Basically, we tell the level set driver that we want a restart
  m_Driver->GetSNAPImageData()->RestartSegmentation();

  // Update the display
  OnSnakeUpdate();
}

void fnSnakeIdleFunction(void *userData)
{
  // Get the instance of the calling class
  UserInterfaceLogic *uiLogic = (UserInterfaceLogic *) userData;

  // Request that the desired number of iterations be executed
  uiLogic->GetDriver()->GetSNAPImageData()->RunSegmentation(
    uiLogic->m_SnakeStepSize);
  
  // Update the display
  uiLogic->OnSnakeUpdate();
}

void 
UserInterfaceLogic
::OnSnakeStopAction()
{
  Fl::remove_idle(fnSnakeIdleFunction, this);
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_EDITABLE, true);
}

void 
UserInterfaceLogic
::OnSnakePlayAction()
{
  m_Activation->UpdateFlag(UIF_SNAP_SNAKE_RUNNING, true);
  Fl::add_idle(fnSnakeIdleFunction, this);
}

void 
UserInterfaceLogic
::OnSnakeUpdate()
{
  // Update the number of elapsed iterations
  std::ostringstream oss;
  oss << m_Driver->GetSNAPImageData()->GetElapsedSegmentationIterations();
  m_OutCurrentIteration->value(oss.str().c_str());

  // Update the mesh if necessary
  if(m_ChkContinuousView3DUpdate->value())
    m_SNAPWindowManager3D->UpdateMesh(m_ProgressCommand);
  else
    m_Activation->UpdateFlag(UIF_SNAP_MESH_DIRTY, true);

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnSnakeStepAction()
{
  // Stop the snake if it's running
  OnSnakeStopAction();

  // Call the idle function directly
  fnSnakeIdleFunction(this);
}

void 
UserInterfaceLogic
::OnSnakeStepSizeChange()
{
  // Save the step size
  m_SnakeStepSize = atoi(m_InStepSize->text());
}


//--------------------------------------------
//
//
// SWITCHING BETWEEN WINDOWS STUFF
//
//
//--------------------------------------------

void 
UserInterfaceLogic
::SyncSnakeToIRIS()
{
  m_InSNAPLabelOpacity->value(m_InIRISLabelOpacity->value());
  // Contrast_slider_s->value(Contrast_slider->value());
  // Brightness_slider_s->value(Brightness_slider->value());

  switch (m_GlobalState->GetToolbarMode()) 
    {
  case(NAVIGATION_MODE):
    m_BtnSNAPNavigation->setonly();
    break;
  default:
    SetToolbarMode(CROSSHAIRS_MODE);
    m_BtnSNAPCrosshairs->setonly();
    break;
    }
}

void 
UserInterfaceLogic
::SyncIRISToSnake()
{
  m_InIRISLabelOpacity->value(m_InSNAPLabelOpacity->value());
  // Contrast_slider->value(Contrast_slider_s->value());
  // Brightness_slider->value(Brightness_slider_s->value());

  switch (m_GlobalState->GetToolbarMode()) 
    {
  case(NAVIGATION_MODE):
    m_BtnNavigationMode->setonly();
    break;
  default:
    SetToolbarMode(CROSSHAIRS_MODE);
    m_BtnCrosshairsMode->setonly();
    break;
    }
}

void 
UserInterfaceLogic
::CloseSegmentationCommon()
{
  // This makes no sense if there is no SNAP
  assert(m_Driver->GetSNAPImageData());

  // Clean up SNAP image data
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseSNAPImageData();

  // Inform the global state that we're not in sNAP
  m_GlobalState->SetSNAPActive(false);

  // Speed image is no longer visible
  m_GlobalState->SetSpeedValid(false);
  m_GlobalState->SetShowSpeed(false);
  m_GlobalState->SetSnakeActive(false);

  // Updates some UI components (?)
  SyncIRISToSnake();

  // Reset the mesh display
  m_IRISWindowManager3D->ClearScreen();
  m_IRISWindowManager3D->ResetView();

  // We are going to preserve the cursor position if it was in the ROI
  Vector3ui newCursor, oldCursor = m_Driver->GetCursorPosition();

  // Image geometry has changed. This method also resets the cursor 
  // position, which is something we don't want.
  OnImageGeometryUpdate();

  // So we reset the cursor position manually here if the cursor was in the ROI
  SNAPSegmentationROISettings roi = m_GlobalState->GetSegmentationROISettings();
  roi.TransformROIVoxelToImageVoxel(oldCursor, newCursor);  
  m_Driver->SetCursorPosition(newCursor);
  this->OnCrosshairPositionUpdate();

  // Clear the list of bubbles
  m_GlobalState->SetBubbleArray(GlobalState::BubbleArray());
  m_GlobalState->SetActiveBubble(-1);

  // Activate/deactivate menu items
  // TODO: build a better state machine
  if(m_Activation->GetFlag(UIF_GRAY_LOADED))
    m_Activation->UpdateFlag(UIF_IRIS_WITH_GRAY_LOADED, true);
  else
    m_Activation->UpdateFlag(UIF_IRIS_WITH_BASEIMG_LOADED, true);

  // The segmentation has changed, so the mesh should be updatable
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
  
  // Hide the color map window
  m_WinColorMap->hide();

  // Restore the SNAP view to four side-by-side windows
  OnWindowFocus(-1);
  
  // Show IRIS window, Hide the snake window
  ShowIRIS();

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnAcceptSegmentationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // Turn off segmentation if it's active
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Get data from SNAP back into IRIS
  m_Driver->UpdateIRISWithSnapImageData(m_ProgressCommand);

  // This is a safeguard in case the progress events do not fire
  m_WinProgress->hide();

  // Create an undo point
  this->StoreUndoPoint("Automatic segmentation");

  // Close up SNAP
  this->CloseSegmentationCommon();
}

void 
UserInterfaceLogic
::OnITKProgressEvent(itk::Object *source, const EventObject &)
{
  // Get the elapsed progress
  itk::ProcessObject *po = reinterpret_cast<ProcessObject *>(source);
  float progress = po->GetProgress();

  // Update the progress bar and value
  m_OutProgressMeter->value(100 * progress);
  m_OutProgressCounter->value(100 * progress);

  // Show or hide progress bar if necessary
  if(progress < 1.0f && !m_WinProgress->visible())
    {
    this->CenterChildWindowInMainWindow(m_WinProgress);
    m_WinProgress->show();
    }
  else if (progress == 1.0f && m_WinProgress->visible())
    {
    m_WinProgress->hide();
    }

  // Update the screen
  Fl::check();
}

void 
UserInterfaceLogic
::OnCancelSegmentationAction()
{
  // Stop the segmentation if it's running
  OnSnakeStopAction();
  
  // This callback has double functionality, depending on whether the 
  // level set update loop is active or not
  if(m_Driver->GetSNAPImageData()->IsSegmentationActive())
    {
    // Tell the update loop to terminate
    m_Driver->GetSNAPImageData()->TerminateSegmentation();
    }

  // Clean up SNAP image data
  CloseSegmentationCommon();
}

void 
UserInterfaceLogic
::OnMenuQuit()
{
  this->OnMainWindowCloseAction();
}

void 
UserInterfaceLogic
::OnMenuImageInfo()
{
  m_WinImageInfo->show();
  CenterChildWindowInMainWindow(m_WinImageInfo);
}

void
UserInterfaceLogic
::OnMenuReorientImage()
{
  m_DlgReorientImage->ShowDialog();  
}

void 
UserInterfaceLogic
::OnCloseImageInfoAction()
{
  m_WinImageInfo->hide();
}

void 
UserInterfaceLogic
::OnImageInformationVoxelCoordinatesUpdate()
{
  // Read the cursor values
  Vector3ui cpos;
  for(size_t i = 0; i < 3; i++)
    {
    cpos[i] = (unsigned int) m_InImageInfoCursorIndex[i]->clamp(
      m_InImageInfoCursorIndex[i]->round(
        m_InImageInfoCursorIndex[i]->value()));
    }
  m_Driver->SetCursorPosition(cpos);
  OnCrosshairPositionUpdate();
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnMainWindowCloseAction()
{
  // Make sure the user doesn't lose any data
  if(!PromptBeforeLosingChanges(REASON_QUIT)) return;

  // We don't want to just exit when users press escape
  if(Fl::event_key() == FL_Escape) return;

  // Check if there have been unsaved changes to the segmentation
  // TODO:

  // Associate the current state with the current image
  OnGreyImageUnload();

  // Create an array of open windows
  vector<Fl_Window *> openWindows;
  openWindows.push_back(Fl::first_window());

  // Add all the open windows to the list
  while(true)
    {
    Fl_Window *win = Fl::next_window(openWindows.back());
    if(win && win != openWindows.front())
      openWindows.push_back(win);
    else break;
    }

  // Close all the windows
  for(unsigned int i=0;i<openWindows.size();i++)
    openWindows[i]->hide();
}

void 
UserInterfaceLogic
::GlobalIdleHandler(void *userData)
{
  // Need base image for all of this
  if(m_GlobalUI->m_Activation->GetFlag(UIF_BASEIMG_LOADED))
    {

    // Read the IPC message
    SystemInterface::IPCMessage ipcm;
    if(m_GlobalUI->m_SystemInterface->IPCRead(ipcm))
      {
      // Update the cursor
      if(m_GlobalUI->m_BtnSynchronizeCursor->value())
        {
        // Map the cursor position to the image coordinates
        GenericImageData *id = m_GlobalUI->m_Driver->GetCurrentImageData();
        Vector3d vox = 
          id->GetGrey()->TransformNIFTICoordinatesToVoxelIndex(ipcm.cursor);

        // Round the cursor to integer value
        itk::Index<3> pos; Vector3ui vpos; 
        pos[0] = vpos[0] = (unsigned int) (vox[0] + 0.5);
        pos[1] = vpos[1] = (unsigned int) (vox[1] + 0.5);
        pos[2] = vpos[2] = (unsigned int) (vox[2] + 0.5);
        
        // Check if the voxel position is inside the image region
        if(vpos != m_GlobalUI->m_Driver->GetCursorPosition() 
          && id->GetImageRegion().IsInside(pos))
          {      
          m_GlobalUI->m_Driver->SetCursorPosition(vpos);
          m_GlobalUI->OnCrosshairPositionUpdate(false);
          m_GlobalUI->RedrawWindows();
          }
        }

      // Update the 3D trackball
      if(m_GlobalUI->m_BtnSynchronizeCursor->value())
        {
        // Get the current 3D window object
        Window3D *w3d = 
          m_GlobalUI->m_GlobalState->GetSNAPActive() 
          ? m_GlobalUI->m_SNAPWindowManager3D 
          : m_GlobalUI->m_IRISWindowManager3D;

        // Compare the two trackballs
        Trackball tball = w3d->GetTrackball();
        char *p1 = reinterpret_cast<char *>(&tball);
        char *p2 = reinterpret_cast<char *>(&ipcm.trackball);
        if(!std::equal(p1,p1+sizeof(Trackball),p2))
          {
          w3d->SetTrackball(ipcm.trackball);
          m_GlobalUI->RedrawWindows();
          }
        }

      // Update the zoom factor (IRIS mode only)
      if(m_GlobalUI->m_ChkMultisessionZoom->value() 
        && m_GlobalUI->m_Activation->GetFlag(UIF_IRIS_ACTIVE))
        {
        if(ipcm.zoom_level != m_GlobalUI->m_SliceCoordinator->GetCommonZoomLevel())
          {
          m_GlobalUI->m_SliceCoordinator->SetZoomLevelAllWindows(ipcm.zoom_level);
          m_GlobalUI->OnZoomUpdate(false);
          }
        }
      }
    }

  Fl::repeat_timeout(0.03, &UserInterfaceLogic::GlobalIdleHandler);
}

int 
UserInterfaceLogic
::GlobalEventHandler(int ev)
{
  return m_GlobalUI->OnGlobalEvent(ev);
}

int 
UserInterfaceLogic
::OnGlobalEvent(int ev)
{
  if(ev == FL_SHORTCUT)
    {
    // Opacity slider toggle/increase/decrease
    if(m_Activation->GetFlag(UIF_IRIS_WITH_BASEIMG_LOADED))
      {
      if(Fl::event_key() == 'a')
        {
        double opacity = m_InIRISLabelOpacity->value() - 8.0;
        if(opacity >= 0.0)
          m_InIRISLabelOpacity->value(opacity); 
        OnIRISLabelOpacityChange();
        return 1;
        }
      else if(Fl::event_key() == 'd')
        {
        double opacity = m_InIRISLabelOpacity->value() + 8.0;
        if(opacity <= 255.0)
          m_InIRISLabelOpacity->value(opacity); 
        OnIRISLabelOpacityChange();
        return 1;
        }
      else if(Fl::event_key() == 's')
        {
        double opacity = m_InIRISLabelOpacity->value();
        if(opacity > 0)
          {
          m_OpacityToggleValue = opacity;
          m_InIRISLabelOpacity->value(0);
          }
        else
          {
          m_InIRISLabelOpacity->value(m_OpacityToggleValue);
          m_OpacityToggleValue = 128;
          }
        OnIRISLabelOpacityChange(); 
        return 1;
        }
      else if(Fl::event_key() == 'i' && Fl::event_alt())
        {
        m_IntensityCurveUI->DisplayWindow();
        m_IntensityCurveUI->OnAutoFitWindow();
        m_IntensityCurveUI->OnClose();
        }
      }
    }
  return 0;
}

void
UserInterfaceLogic
::Launch()
{
  // Add the global event handler
  UserInterfaceLogic::m_GlobalUI = this;
  Fl::add_handler(&UserInterfaceLogic::GlobalEventHandler);

  // Add the idle event handler
  Fl::add_timeout(0.03, &UserInterfaceLogic::GlobalIdleHandler);

  // Make sure the window is visible
  m_WinMain->show();

  // Show all of the GL boxes
  for(unsigned int i = 0; i < 3; i++)
    m_SliceWindow[i]->show();
  m_RenderWindow->show();

  // Show the IRIS interface
}

void 
UserInterfaceLogic
::ShowIRIS()
{
  // Show the right wizard page
  m_WizControlPane->value(
    m_Driver->GetCurrentImageData()->IsGreyLoaded() ? 
    m_GrpToolbarPage : m_GrpWelcomePage);

  // Show the right toolbar page under the render window
  m_WizRenderingToolbar->value(m_GrpRenderingIRISPage);

  // Assign the right window managers to the slice windows
  for(unsigned int i = 0; i < 3; i++)
    m_SliceWindow[i]->SetSingleInteractionMode(m_IRISWindowManager2D[i]);
  m_RenderWindow->SetSingleInteractionMode(m_IRISWindowManager3D);

  // Clear the 3D window and reset the view
  m_IRISWindowManager3D->ClearScreen();
  m_IRISWindowManager3D->ResetView();

  // Force a global redraw
  RedrawWindows();
}

void 
UserInterfaceLogic
::ShowSNAP()
{
  // Restore the IRIS view to four side-by-side windows
  OnWindowFocus(-1);

  // Swap the left-side panels
  m_WizControlPane->value(m_GrpSNAPPage);

  // Show the right toolbar page under the render window
  m_WizRenderingToolbar->value(m_GrpRenderingSNAPPage);

  // Assign the right window managers to the slice windows
  for(unsigned int i = 0; i < 3; i++)
    m_SliceWindow[i]->SetSingleInteractionMode(m_SNAPWindowManager2D[i]);
  m_RenderWindow->SetSingleInteractionMode(m_SNAPWindowManager3D);

  // Clear the snap window and reset the view
  m_SNAPWindowManager3D->ClearScreen();
  m_SNAPWindowManager3D->ResetView();

  // Indicate that preprocessing is not done
  m_Activation->UpdateFlag(UIF_SNAP_ACTIVE, true);
  m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, false);

  // Go to the first page in the SNAP wizard
  SetActiveSegmentationPipelinePage( 0 );

  // Repaint everything
  RedrawWindows();
}

void 
UserInterfaceLogic
::InitializeUI()
{
  // Make the menu bar global
  m_MenubarMain->global();

  // Set the version text in the welcome page
  m_InWelcomePageVersion->label(SNAPUISoftVersion);
  m_InAboutPageVersion->label(SNAPUISoftVersion);

  // Set local variables
  m_GreyFileLoaded = 0;
  m_RGBFileLoaded = 0;
  m_SegmentationLoaded = 0;

  // Sync global state to GUI
  m_BtnCrosshairsMode->setonly();
  // SetToolbarMode(CROSSHAIRS_MODE);

  m_GlobalState->SetSegmentationAlpha(128);
  m_InIRISLabelOpacity->Fl_Valuator::value(128);

  m_GlobalState->SetRGBAlpha(128);
  m_RGBOverlayUI->SetOpacity(128);
  
  // Initialize the color map for the first time
  UpdateColorLabelMenu();

  // Window title
  this->UpdateMainLabel();

  m_GlobalState->SetShowSpeed(false);

  m_InSNAPLabelOpacity->Fl_Valuator::value(128);

  //this should probably go into the .h, a #define or something
  m_InStepSize->add("1");
  m_InStepSize->add("2");
  m_InStepSize->add("5");
  m_InStepSize->add("10");

  // Initialize the toolbar at the bottom of 3D window
  m_ToolbarRenderWindow->spacing(5);

  // Apply the special appearance settings that determine startup behavior
  if(m_AppearanceSettings->GetFlagLinkedZoomByDefault())
    {
    m_ChkLinkedZoom->value(1);    
    OnLinkedZoomChange();
    }

  // Initialize the paintbrush panel
  this->UpdatePaintbrushAttributes();
}

void 
UserInterfaceLogic
::UpdateColorLabelSelection()
{
  // Set the current drawing label
  LabelType iDrawing = m_GlobalState->GetDrawingColorLabel();
  LabelType iDrawOver = m_GlobalState->GetOverWriteColorLabel();

  // Select the drawing label
  for(size_t i = 0; i < static_cast<size_t>(m_InDrawingColor->size()); i++)
    if(iDrawing == static_cast<LabelType>(reinterpret_cast<size_t>(m_InDrawingColor->menu()[i].user_data())))
      m_InDrawingColor->value(i);

  // Set the draw over label
  if(m_GlobalState->GetCoverageMode() == PAINT_OVER_ALL)
    m_InDrawOverColor->value(0);
  else if (m_GlobalState->GetCoverageMode() == PAINT_OVER_COLORS)
    m_InDrawOverColor->value(1);
  else for(size_t j = 0; j < static_cast<size_t>(m_InDrawingColor->size()); j++)
    if(iDrawOver == static_cast<LabelType>(reinterpret_cast<size_t>( m_InDrawingColor->menu()[j].user_data())))
      m_InDrawOverColor->value(j + 2);
}

void
UserInterfaceLogic
::DeleteColorLabelMenu(Fl_Menu_Item *menu)
{
  if(menu) 
    {
    for(Fl_Menu_Item *mi = menu; mi->text != NULL; ++mi)
      delete mi->text;
    delete menu;
    }
}

void InitMenuItem(Fl_Menu_Item *p, const ColorLabel &cl, LabelType id)
{
  char *strcopy = (char *) calloc(strlen(cl.GetLabel()) + 10, sizeof(char));
  sprintf(strcopy, "%08x %s", 
    fl_rgb_color(cl.GetRGB(0),cl.GetRGB(1),cl.GetRGB(2)),
    cl.GetLabel()); 
  p->label(COLORBAR_LABEL, strcopy);
  p->shortcut(0);
  p->callback((Fl_Callback *) 0);
  p->user_data((void *) (size_t) id);
  p->flags = 0;     
  p->labelcolor(FL_BLACK);
  p->labelfont(0);
  p->labelsize(0);
}

void InitMenuItem(Fl_Menu_Item *p, const char *text)
{
  char *strcopy = (char *) calloc(strlen(text) + 1, sizeof(char));
  strcpy(strcopy, text); 
  p->label(FL_NORMAL_LABEL, strcopy);
  p->shortcut(0);
  p->callback((Fl_Callback *) 0);
  p->user_data(0);
  p->flags = 0;     
  p->labelcolor(FL_BLACK);
  p->labelfont(0);
  p->labelsize(0);
}


Fl_Menu_Item *
UserInterfaceLogic
::GenerateColorLabelMenu(bool all, bool visible, bool clear) 
{
  // Compute the number of labels
  ColorLabelTable *clt = m_Driver->GetColorLabelTable();
  size_t n = clt->GetNumberOfValidLabels() - 1;
  if(all) n++;
  if(visible) n++;
  if(clear) n++;

  // Create the new menu and a pointer
  Fl_Menu_Item *menu = new Fl_Menu_Item[n+1], *p = menu;

  // Add the all labels item
  if(all)
    InitMenuItem(p++, "All labels");
  if(visible)
    InitMenuItem(p++, "Visible labels");
  if(clear)
    InitMenuItem(p++, clt->GetColorLabel(0), 0);
  for (size_t i = 1; i < MAX_COLOR_LABELS; i++) 
    {
    const ColorLabel &cl = clt->GetColorLabel(i);
    if (cl.IsValid()) 
      InitMenuItem(p++, cl, i);
    }

  // Last item is NULL
  p->text = NULL;

  // Return the menu
  return menu;
}

void 
UserInterfaceLogic
::UpdateColorLabelMenu() 
{  
  // Set up the paint-over menu
  m_InDrawOverColor->clear();
  DeleteColorLabelMenu(m_MenuDrawOverLabels);
  m_MenuDrawOverLabels = GenerateColorLabelMenu(true, true, true);
  m_InDrawOverColor->menu(m_MenuDrawOverLabels);

  // Set up the draw color menu
  m_InDrawingColor->clear();
  DeleteColorLabelMenu(m_MenuDrawingLabels);
  m_MenuDrawingLabels = GenerateColorLabelMenu(false, false, true);
  m_InDrawingColor->menu(m_MenuDrawingLabels);
  
  // Update the color label that is currently selected
  UpdateColorLabelSelection();
}

void 
UserInterfaceLogic
::RedrawWindows() 
{
  // Redraw the OpenGL windows
  m_SliceWindow[0]->redraw();
  m_SliceWindow[1]->redraw();
  m_SliceWindow[2]->redraw();
  m_RenderWindow->redraw();

  // TODO: Do we really need this?
  // Redraw the current color swath (?)
  if(m_GrpSNAPCurrentColor->visible())
    m_GrpSNAPCurrentColor->redraw();
}

void 
UserInterfaceLogic
::ResetScrollbars() 
{
  // Get the cursor position in image coordinated
  Vector3d cursor = to_double(m_Driver->GetCursorPosition());

  // Update the correct scroll bars
  for (unsigned int dim=0; dim<3; dim++)
    {
    // What image axis does dim correspond to?
    unsigned int imageAxis = GetImageAxisForDisplayWindow(dim);
    m_InSliceSlider[dim]->Fl_Valuator::value( -cursor[imageAxis] );

    // Update the little display box at the bottom of the scroll bar
    UpdatePositionDisplay(dim);
    }
}

void 
UserInterfaceLogic
::OnCrosshairPositionUpdate(bool flagBroadcastUpdate)
{
  this->ResetScrollbars();
  this->UpdateImageProbe();

  // When the crosshair position is updated, we send the information 
  // to the inter-process communications system
  if(flagBroadcastUpdate 
    && m_GlobalUI->m_Activation->GetFlag(UIF_BASEIMG_LOADED)
    && m_GlobalUI->m_BtnSynchronizeCursor->value())
    {
    // Map the cursor to NIFTI coordinates
    Vector3d cursor = 
      m_Driver->GetCurrentImageData()->GetGrey()->
        TransformVoxelIndexToNIFTICoordinates(
          m_Driver->GetCursorPosition());

    // Write the NIFTI cursor to shared memory
    m_Driver->GetSystemInterface()->IPCBroadcastCursor(cursor);
    }
}

void
UserInterfaceLogic
::OnTrackballUpdate(bool flagBroadcastUpdate)
{
  if(flagBroadcastUpdate
    && m_GlobalUI->m_Activation->GetFlag(UIF_BASEIMG_LOADED)
    && m_GlobalUI->m_BtnSynchronizeCursor->value())
    {
    Trackball tball = 
      m_GlobalState->GetSNAPActive() ? 
      m_SNAPWindowManager3D->GetTrackball() : 
      m_IRISWindowManager3D->GetTrackball();
    m_Driver->GetSystemInterface()->IPCBroadcastTrackball(tball);
    }
}

void 
UserInterfaceLogic
::OnDrawingLabelUpdate()
{
  // Get the drawing label that was selected
  LabelType iLabel = (LabelType) (size_t) 
    m_InDrawingColor->mvalue()->user_data();
  
  // Set the global state
  m_GlobalState->SetDrawingColorLabel((LabelType) iLabel);
}

void 
UserInterfaceLogic
::OnDrawOverLabelUpdate()
{
  // See what the user selected
  if(m_InDrawOverColor->value() == 0)
    // The first menu item is 'paint over all'
    m_GlobalState->SetCoverageMode(PAINT_OVER_ALL);
  else if(m_InDrawOverColor->value() == 1)
    // The first menu item is 'paint over visible'
    m_GlobalState->SetCoverageMode(PAINT_OVER_COLORS);
  else
    {
    LabelType iLabel = (LabelType) (size_t) 
      m_InDrawOverColor->mvalue()->user_data();
    m_GlobalState->SetCoverageMode(PAINT_OVER_ONE);
    m_GlobalState->SetOverWriteColorLabel(iLabel);
    }
}

void 
UserInterfaceLogic
::UpdateImageProbe() 
{
  // Code common to SNAP and IRIS
  Vector3ui crosshairs = m_Driver->GetCursorPosition();
    
  // String streams for different labels
  IRISOStringStream sGrey,sSegmentation,sSpeed;

  // Get the grey intensity
  GenericImageData *id = m_Driver->GetCurrentImageData();
  sGrey << id->GetGrey()->GetVoxelMappedToNative(crosshairs);

  // Get the segmentation lavel intensity
  int iSegmentation = (int) id->GetSegmentation()->GetVoxel(crosshairs);
  sSegmentation << iSegmentation;

  // Update the cursor position in the image info window
  Vector3d xPosition = id->GetGrey()->TransformVoxelIndexToPosition(crosshairs);

  Vector3d xNIFTI = id->GetGrey()->TransformVoxelIndexToNIFTICoordinates(crosshairs);

  for(size_t d = 0; d < 3; d++)
    {
    m_InImageInfoCursorIndex[d]->value(crosshairs[d]);
    m_OutImageInfoCursorPosition[d]->value(xPosition[d]);
    m_OutImageInfoCursorNIFTIPosition[d]->value(xNIFTI[d]);
    }
  
  // The rest depends on the current mode
  if(m_GlobalState->GetSNAPActive())
    {
    // Fill the grey and label outputs
    m_OutSNAPProbe->value(sGrey.str().c_str());
    m_OutSNAPLabelProbe->value(sSegmentation.str().c_str());

    // Get a pointer to the speed image wrapper
    SNAPImageData *snap = m_Driver->GetSNAPImageData();

    // Get the speed value (if possible)
    if(m_GlobalState->GetSpeedPreviewValid())
      {
      // Speed preview is being shown.  Get a preview pixel
      sSpeed << std::setprecision(4) << 
        snap->GetSpeed()->GetPreviewVoxel(crosshairs);
      }
    else if(m_GlobalState->GetSpeedValid())
      {
      // Speed image is valid, i.e., has been properly computed
      sSpeed << std::setprecision(4) << 
        snap->GetSpeed()->GetVoxel(crosshairs);
      }
    else 
      {
      sSpeed << "N/A";
      }

    // Display the speed string
    m_OutSNAPSpeedProbe->value(sSpeed.str().c_str());
    }
  else
    {
    m_OutGreyProbe->value(sGrey.str().c_str());
    m_OutLabelProbe->value(sSegmentation.str().c_str());

    // Get the label description
    ColorLabel cl = 
      m_Driver->GetColorLabelTable()->GetColorLabel(iSegmentation);
    m_OutLabelProbeText->value(cl.GetLabel());      
    }
}

void 
UserInterfaceLogic
::UpdateMainLabel() 
{
  // Get the main image name
  string fnMain = "";
  if(m_Activation->GetFlag(UIF_GRAY_LOADED))
    fnMain = m_GlobalState->GetGreyFileName();
  else if(m_Activation->GetFlag(UIF_RGBBASE_LOADED))
    fnMain = m_GlobalState->GetRGBFileName();

  // Truncate the main image file    
  if(fnMain.length())
    fnMain = itksys::SystemTools::GetFilenameName(fnMain);
  else
    fnMain = "[No Image]";

  // Get the segmentation file name
  string fnSeg = m_GlobalState->GetSegmentationFileName();
  if(fnSeg.length())
    fnSeg = itksys::SystemTools::GetFilenameName(fnSeg);
  else
    fnSeg = "[New Segmentation]";

  // Build up the label
  std::ostringstream oss;

  // Print the main image
  oss << fnMain;
  
  // TODO: print if the main image is dirty

  // Print the segmentation image
  if(fnMain != "[No Image]")
    {
    oss << " - " << fnSeg;

    // Print the * if image is dirty
    if(m_Activation->GetFlag(UIF_UNSAVED_CHANGES))
      oss << "*";
    }

  // Print program name and version
  oss << " - " << SNAPSoftVersion;

  // Store the label
  m_MainWindowLabel = oss.str();

  // Apply to the window
  m_WinMain->label(m_MainWindowLabel.c_str());

  return;
}

void 
UserInterfaceLogic
::OnEditLabelsAction()
{
  // Get the currently selected color index
  LabelType iLabel = (LabelType) (size_t) 
    m_InDrawingColor->mvalue()->user_data();
  m_LabelEditorUI->OnLabelListUpdate(iLabel);

  // Display the label editor
  m_LabelEditorUI->DisplayWindow();
}

void 
UserInterfaceLogic
::UpdateEditLabelWindow() 
{
  /** 
  // Get properties from VoxData
  int index = m_ColorMap[m_InDrawingColor->value()];

  // Get the color label
  ColorLabel cl = m_Driver->GetCurrentImageData()->GetColorLabel(index);
  assert(cl.IsValid());

  // Set widgets in EditLabel window
  m_InEditLabelName->value(cl.GetLabel());
  m_InEditLabelAlpha->value(cl.GetAlpha() / 255.0);
  m_InEditLabelVisibility->value(!cl.IsVisible());
  m_InEditLabelMesh->value(!cl.IsVisibleIn3D());
  m_BtnEditLabelChange->setonly();

  // convert from uchar [0,255] to double [0.0,1.0]
  m_GrpEditLabelColorChooser->rgb( cl.GetRGB(0)/255.0, cl.GetRGB(1)/255.0, cl.GetRGB(2)/255.0 );

  // If this is clear label, assume we're going to add a label
  if (index == 0) 
    {
    m_BtnEditLabelAdd->setonly();
    m_InEditLabelName->value("New Label");
    m_InEditLabelAlpha->value(1.0);
    m_InEditLabelVisibility->clear();
    m_InEditLabelMesh->clear();
    }

  char title[100];
  sprintf(title, "Label No. %d", index);
  m_WinEditLabel->label(title);
  */
  
  
}

/*
void 
UserInterfaceLogic
::ChangeLabelsCallback() 
{
  // Check the new label name
  const char* new_name = m_InEditLabelName->value();
  if (strlen(new_name) == 0) 
    {
    m_OutMessage->value("You must enter a non-null name");
    return;
    }

  // Update the label Choice widgets
  int offset;
  if (m_BtnEditLabelAdd->value() == 1) 
    {  // add a new label
    offset = m_InDrawingColor->add(new_name);
    m_InDrawOverColor->add(new_name);   
    }
  else 
    {  // Replace an old label
    offset = m_InDrawingColor->value();
    if (!offset) 
      {
      m_OutMessage->value("You cannot recolor the clear color");
      return;
      }
    m_InDrawingColor->replace(offset, new_name);
    m_InDrawOverColor->replace(offset+2, new_name);
    }
  m_InDrawingColor->value(offset);
  m_InDrawingColor->set_changed();

  // Search for a place to put this new label
  if (m_ColorMap[offset] <0) 
    {
    int i;
    for (i=1; i<256 && m_Driver->GetCurrentImageData()->GetColorLabel(i).IsValid(); i++) 
      {
      }

    m_ColorMap[offset] = i;
    }

  unsigned char rgb[3];
  rgb[0] = (uchar) ( 255*m_GrpEditLabelColorChooser->r() );
  rgb[1] = (uchar) ( 255*m_GrpEditLabelColorChooser->g() );
  rgb[2] = (uchar) ( 255*m_GrpEditLabelColorChooser->b() );

  // Send changes to the Voxel Data Structure
  ColorLabel cl = m_Driver->GetCurrentImageData()->GetColorLabel(m_ColorMap[offset]);
  cl.SetRGBVector(rgb);
  cl.SetAlpha(static_cast<unsigned char>(255 * m_InEditLabelAlpha->value()));
  cl.SetVisible(!m_InEditLabelVisibility->value());
  cl.SetVisibleIn3D(!m_InEditLabelMesh->value());
  cl.SetLabel(new_name);
  cl.SetValid(true);

  // Make sure that the display windows get the updated color labels
  m_Driver->GetCurrentImageData()->SetColorLabel(m_ColorMap[offset],cl);

  // Set the new current color in the GUI
  m_GrpCurrentColor->color(fl_rgb_color(rgb[0], rgb[1], rgb[2]));

  m_GlobalState->SetDrawingColorLabel((unsigned char) m_ColorMap[offset]);

  // The segmentation has become dirty
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

  // Redraw the windows 
  this->RedrawWindows();
  m_WinMain->redraw();
}
*/

/** 
 * This method is executed when the labels have been changed by the label editor
 * or by loading them from file
 */
void
UserInterfaceLogic
::OnLabelListUpdate()
{
  // Update the drop down box of labels to reflect the current settings
  UpdateColorLabelMenu();

  // The label wrappers depend on color maps
  m_Driver->GetCurrentImageData()->GetSegmentation()->UpdateColorMappingCache();
  
  // Repaint the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::OnSliceSliderChange(int id) 
{
  // Get the new value depending on the current state
  unsigned int value = (unsigned int) ( - m_InSliceSlider[id]->value());
  
  // Get the cursor position
  Vector3ui cursor = m_Driver->GetCursorPosition();

  // Determine which image axis the display window 'id' corresponds to
  unsigned int imageAxis = GetImageAxisForDisplayWindow(id);

  // Update the cursor
  cursor[imageAxis] = value;
  m_Driver->SetCursorPosition(cursor);
  this->OnCrosshairPositionUpdate();

  // Update the little display box under the scroll bar
  UpdatePositionDisplay(id);

  // Update the probe values
  UpdateImageProbe();

  // Repaint the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::UpdatePositionDisplay(int id) 
{
  // Dump out the slice index of the given window
  IRISOStringStream sIndex;
  sIndex << std::setw(4) << (1 - m_InSliceSlider[id]->value());
  sIndex << " of ";
  sIndex << (1 - m_InSliceSlider[id]->minimum());
  m_OutSliceIndex[id]->value(sIndex.str().c_str());
}

void 
UserInterfaceLogic
::OnAcceptPolygonAction(unsigned int window)
{
  if(m_IRISWindowManager2D[window]->AcceptPolygon())
    {
    // The polygon update was successful
    OnPolygonStateUpdate(window);

    // The segmentation has been dirtied
    m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

    // Set the undo point
    this->StoreUndoPoint("Polygon drawing");
    }
  else
    {
    // Probably, the draw-over-label is set incorrectly!
    fl_message(
      "The segmentation was not updated. Most likely, the 'Draw Over' label \n"
      "is set incorrectly. Change it to 'All Labels' or 'Clear Label'.");
    }
}

void 
UserInterfaceLogic
::OnInsertIntoPolygonSelectedAction(unsigned int window)
{
  m_IRISWindowManager2D[window]->InsertPolygonPoints();  
  OnPolygonStateUpdate(window);
}

void 
UserInterfaceLogic
::OnDeletePolygonSelectedAction(unsigned int window)
{
  m_IRISWindowManager2D[window]->DeleteSelectedPolygonPoints();
  OnPolygonStateUpdate(window);
}

void 
UserInterfaceLogic
::OnPastePolygonAction(unsigned int window)
{
  m_IRISWindowManager2D[window]->PastePolygon();
  OnPolygonStateUpdate(window);
}

void
UserInterfaceLogic
::OnIRISFreehandFittingRateUpdate()
{
  if(m_InIRISFreehandContinuous->value())
    {
    m_InIRISFreehandFittingRate->deactivate();
    for(size_t i = 0; i < 3; i++)
      {
      m_IRISWindowManager2D[i]->SetFreehandFittingRate(0);  
      }
    }
  else
    {
    m_InIRISFreehandFittingRate->activate();
    for(size_t i = 0; i < 3; i++)
      {
      m_IRISWindowManager2D[i]->SetFreehandFittingRate(
        m_InIRISFreehandFittingRate->value());
      }
    }
}

void 
UserInterfaceLogic
::OnPolygonStateUpdate(unsigned int id)
{
  // Get the drawing object
  PolygonDrawing *draw = m_IRISWindowManager2D[id]->GetPolygonDrawing();

  if (draw->GetState() == PolygonDrawing::INACTIVE_STATE) 
    {
    // There is no active polygon
    m_BtnPolygonAccept[id]->deactivate();
    m_BtnPolygonDelete[id]->deactivate();
    m_BtnPolygonInsert[id]->deactivate();

    // Check if there is a cached polygon for pasting
    if(draw->GetCachedPolygon()) 
      m_BtnPolygonPaste[id]->activate();
    else
      m_BtnPolygonPaste[id]->deactivate();      
    }
  else if(draw->GetState() == PolygonDrawing::DRAWING_STATE)
    {
    // There is no active polygon
    m_BtnPolygonAccept[id]->deactivate();
    m_BtnPolygonDelete[id]->deactivate();
    m_BtnPolygonInsert[id]->deactivate();
    m_BtnPolygonPaste[id]->deactivate();
    }
  else
    {
    m_BtnPolygonAccept[id]->activate();
    m_BtnPolygonPaste[id]->deactivate();

    if(draw->GetSelectedVertices())
      {
      m_BtnPolygonDelete[id]->activate();
      m_BtnPolygonInsert[id]->activate();
      }
    else
      {
      m_BtnPolygonDelete[id]->deactivate();
      m_BtnPolygonInsert[id]->deactivate();
      }
    }

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::UpdatePaintbrushAttributes()
{
  PaintbrushSettings pbs = m_GlobalState->GetPaintbrushSettings();

  // Get the mode (not nice C, but so be it)
  int mode = (int) pbs.mode;
  m_InPaintbrushShape->value(m_ChoicePaintbrush[mode]);
  m_WizPaintbrushParameters->value(m_GrpPaintbrushParameters[mode]);

  // Set the size
  m_InPaintbrushSize->value(pbs.radius * 2.0);
  
  // Set the other parameters
  m_BtnPaintbrushIsotropic->value(pbs.isotropic ? 1 : 0);
  m_BtnPaintbrushChaseMode->value(pbs.chase ? 1 : 0);
  m_BtnPaintbrush3D->value(pbs.flat ? 0 : 1);
  m_InPaintbrushSmoothing->value(pbs.watershed.smooth_iterations);
  m_InPaintbrushGranularity->value(pbs.watershed.level);  
}

// Change the settings of the paintbrush tool
void
UserInterfaceLogic
::OnPaintbrushAttributesUpdate()
{
  // Get the current paintbrush settings
  PaintbrushSettings pbs = m_GlobalState->GetPaintbrushSettings();

  // Update the settings from the GUI
  pbs.radius = 0.5 * m_InPaintbrushSize->value();
  pbs.flat = m_BtnPaintbrush3D->value() ? false : true;
  pbs.isotropic = m_BtnPaintbrushIsotropic->value() ? true : false;
  pbs.chase = m_BtnPaintbrushChaseMode->value() ? true : false;

  pbs.watershed.level = m_InPaintbrushGranularity->value();
  pbs.watershed.smooth_iterations = m_InPaintbrushSmoothing->value();

  // Set the mode
  pbs.mode = (PaintbrushMode) (m_InPaintbrushShape->value());

  // Update the wizard
  m_WizPaintbrushParameters->value(
    m_GrpPaintbrushParameters[m_InPaintbrushShape->value()]);

  // Set the paintbrush attributes
  m_GlobalState->SetPaintbrushSettings(pbs);

  // Update the windows
  RedrawWindows();
}

void
UserInterfaceLogic
::OnPaintbrushPaint()
{
  // The user painted something. We should make it possible to modifu the mesh
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
}

void 
UserInterfaceLogic
::OnMenuShowDisplayOptions()
{
  m_DlgAppearance->ShowDialog();
}

void 
UserInterfaceLogic
::OnMenuShowRGBOverlayOptions()
{
  m_RGBOverlayUI->DisplayWindow();
}

void
UserInterfaceLogic
::UpdateWindowFocus(Fl_Group *parent, Fl_Group **panels, Fl_Gl_Window **boxes, int iWindow)
{
  // The dimensions of the parent window
  int x = parent->x(), y = parent->y();
  int w = parent->w(), h = parent->h();

  // Check if this is an expansion or a collapse operation
  if( iWindow < 0 || panels[iWindow]->w() == w )
    {
    // Restore all panels to original configuration
    panels[0]->resize(x, y, w >> 1, h >> 1);
    panels[1]->resize(x + (w >> 1), y, w - (w >> 1), h >> 1);
    panels[3]->resize(x, y + (h >> 1), w >> 1, h - (h >> 1));
    panels[2]->resize(x + (w >> 1), y + (h >> 1), w - (w >> 1), h - (h >> 1));

    // Make the resizable is set to self
    parent->resizable(parent);

    // Show everything
    for(unsigned int j = 0; j < 4; j++)
      {
      parent->add(panels[j]);
      panels[j]->show();
      boxes[j]->show();
      panels[j]->redraw();
      }

    }
  else 
    {
    for(int j = 0; j < 4; j++)
      {
      if(iWindow != j)
        {
        panels[j]->hide();
        boxes[j]->hide();
        parent->remove(panels[j]);
        }
      }

    panels[iWindow]->resize(
      parent->x(),parent->y(),
      parent->w(),parent->h());
    parent->resizable(panels[iWindow]);
    panels[iWindow]->redraw();
    }
}

void
UserInterfaceLogic
::OnWindowFocus(int iWindow)
{
  // Get the four panels
  Fl_Group *panels[] = 
    { m_GrpSlicePanel[0], m_GrpSlicePanel[1], m_GrpSlicePanel[2], m_GrpView3D };
  Fl_Gl_Window *boxes[] = 
    { m_SliceWindow[0], m_SliceWindow[1], m_SliceWindow[2], m_RenderWindow };

  // Update the window focus
  UpdateWindowFocus(m_GrpRightPane, panels, boxes, iWindow);
}


void
UserInterfaceLogic
::OnImageGeometryUpdate()
{
  if(!m_Driver->GetCurrentImageData()->IsGreyLoaded())
    return;

  // Set the crosshairs to the center of the image
  Vector3ui size = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  Vector3ui xCross = size / ((unsigned int) 2);
  m_Driver->SetCursorPosition(xCross);

  // Update the crosshairs display
  this->OnCrosshairPositionUpdate();

  // Update the source for slice windows as well as scroll bars
  for (unsigned int i=0; i<3; i++) 
    {
    if(m_GlobalState->GetSNAPActive())
      m_SNAPWindowManager2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());
    else
      m_IRISWindowManager2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());

    // Get the image axis that corresponds to the display window i
    unsigned int imageAxis = GetImageAxisForDisplayWindow(i);

    // Notice the sliders have a negative range!  That's so that the 1 position 
    // is at the bottom.  We need to always negate the slider values
    m_InSliceSlider[i]->range( 1.0 - size[imageAxis], 0.0 );
    m_InSliceSlider[i]->slider_size( 1.0/ size[imageAxis] );
    m_InSliceSlider[i]->linesize(1);
    }
      
  // Group the three windows inside the window coordinator
  m_SliceCoordinator->RegisterWindows(
    reinterpret_cast<GenericSliceWindow **>(m_IRISWindowManager2D));    

  // Reset the view in 2D windows to fit
  m_SliceCoordinator->ResetViewToFitInAllWindows();

  // Fire zoom update event
  this->OnZoomUpdate();
}
  
void 
UserInterfaceLogic
::OnMeshResetViewAction()
{
  if(m_GlobalState->GetSNAPActive())
    m_SNAPWindowManager3D->ResetView();
  else
    m_IRISWindowManager3D->ResetView();
  m_RenderWindow->redraw();
}

void 
UserInterfaceLogic
::OnIRISMeshAcceptAction()
{
  m_IRISWindowManager3D->Accept();
  RedrawWindows();
  StoreUndoPoint("3D operation");
  
  m_Activation->UpdateFlag(UIF_IRIS_MESH_ACTION_PENDING, false);
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
}

void 
UserInterfaceLogic
::OnIRISMeshUpdateAction()
{
  // Update the mesh and redraw the window
  m_IRISWindowManager3D->UpdateMesh(m_ProgressCommand);
  m_RenderWindow->redraw();

  // This is a safeguard in case the progress events do not fire
  m_WinProgress->hide();
  
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, false);
}

void
UserInterfaceLogic
::OnIRISMeshEditingAction()
{
  m_Activation->UpdateFlag(UIF_IRIS_MESH_ACTION_PENDING, true);
}

void
UserInterfaceLogic
::OnIRISMeshDisplaySettingsUpdate()
{
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
}

void
UserInterfaceLogic
::OnSNAPMeshUpdateAction()
{
  m_SNAPWindowManager3D->UpdateMesh(m_ProgressCommand);
  m_RenderWindow->redraw();

  // This is a safeguard in case the progress events do not fire
  m_WinProgress->hide();

  m_Activation->UpdateFlag(UIF_SNAP_MESH_DIRTY, false);
}

void 
UserInterfaceLogic
::OnSNAPMeshContinuousUpdateAction()
{
  if (m_ChkContinuousView3DUpdate->value()) 
    {
    m_SNAPWindowManager3D->UpdateMesh(m_ProgressCommand);
    m_Activation->UpdateFlag(UIF_SNAP_MESH_CONTINUOUS_UPDATE, true);
    }
  else 
    {
    m_Activation->UpdateFlag(UIF_SNAP_MESH_CONTINUOUS_UPDATE, false);
    }
}

void 
UserInterfaceLogic
::SetToolbarMode(ToolbarModeType mode)
{
  // There must be an active image before we do this stuff
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Set the mode on the toolbar
  m_GlobalState->SetToolbarMode(mode);

  // Set the mode of each window
  for(unsigned int i=0;i<3;i++)
    {
    switch(mode) 
      {
    case CROSSHAIRS_MODE:
      m_IRISWindowManager2D[i]->EnterCrosshairsMode();
      m_SNAPWindowManager2D[i]->EnterCrosshairsMode();
      m_TabsToolOptions->value(m_GrpToolOptionCrosshairs);
      break;

    case NAVIGATION_MODE:
      m_IRISWindowManager2D[i]->EnterZoomPanMode();
      m_SNAPWindowManager2D[i]->EnterZoomPanMode();
      m_TabsToolOptions->value(m_GrpToolOptionZoomPan);
      break;

    case POLYGON_DRAWING_MODE:
      m_IRISWindowManager2D[i]->EnterPolygonMode();
      m_TabsToolOptions->value(m_GrpToolOptionPolygon);
      break;

    case PAINTBRUSH_MODE:
      m_IRISWindowManager2D[i]->EnterPaintbrushMode();
      m_TabsToolOptions->value(m_GrpToolOptionBrush);
      break;

    case ROI_MODE:
      m_IRISWindowManager2D[i]->EnterRegionMode();
      m_TabsToolOptions->value(m_GrpToolOptionSnAP);
      break;

    default:
      break;  
      }

    // Enable the polygon editing windows
    if(mode == POLYGON_DRAWING_MODE)
      m_GrpPolygonEdit[i]->show();
    else
      m_GrpPolygonEdit[i]->hide();

    }

  // Redraw the windows
  RedrawWindows();
}

void 
UserInterfaceLogic
::SetToolbarMode3D(ToolbarMode3DType mode)
{
  switch (mode)
    {
    case TRACKBALL_MODE:
      m_IRISWindowManager3D->EnterTrackballMode();
      m_SNAPWindowManager3D->EnterTrackballMode();
      break;

    case CROSSHAIRS_3D_MODE:
      m_IRISWindowManager3D->EnterCrosshairsMode();
      m_SNAPWindowManager3D->EnterCrosshairsMode();
      break;

    case SPRAYPAINT_MODE:
      m_IRISWindowManager3D->EnterSpraypaintMode();
      break;

    case SCALPEL_MODE:
      m_IRISWindowManager3D->EnterScalpelMode();
      break;

    default:
      break;  
    }

  // Redraw the 3D windows
  m_RenderWindow->redraw();
}


void 
UserInterfaceLogic
::OnMenuWriteVoxelCounts() 
{
  // Display the load labels dialog
  m_DlgVoxelCountsIO->SetTitle("Save Volume Statistics");
  m_DlgVoxelCountsIO->DisplaySaveDialog(
    m_SystemInterface->GetHistory("VolumeStatistics"));
}

void 
UserInterfaceLogic
::OnWriteVoxelCountsAction() 
{
  // Get the selected file name
  const char *file = m_DlgVoxelCountsIO->GetFileName();

  // Try writing
  try 
    {
    // Compute the statistics and write them to file
    m_Driver->ExportSegmentationStatistics(file);
    
    // Update the history
    m_SystemInterface->UpdateHistory("VolumeStatistics",file);
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error writing volume statistics:\n%s",exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnGreyImageUnload()
{
  // This method is called when a grey image gets unloaded.  It saves the 
  // current settings and associates them with the grey image file
  
  // Make sure there is actually an image
  string fnGrey = m_GlobalState->GetGreyFileName();
  if(fnGrey.length())
    m_SystemInterface->AssociateCurrentSettingsWithCurrentImageFile(
      fnGrey.c_str(),m_Driver);
}

void 
UserInterfaceLogic
::OnGreyImageUpdate()
{
  // Blank the screen - useful on a load of new grey data when there is 
  // already a segmentation file present
  m_IRISWindowManager3D->ClearScreen();

  // Flip over to the toolbar page
  m_WizControlPane->value(m_GrpToolbarPage);

  // Loading a gray image means unloading the RGB image
  m_Activation->UpdateFlag(UIF_RGBANY_LOADED, false);

  // Enable some menu items
  m_Activation->UpdateFlag(UIF_IRIS_WITH_GRAY_LOADED, true);

  // Disable undo and redo 
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());

  // There are now no unsaved changes
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, false);

  // Image geometry has changed
  OnImageGeometryUpdate();

  // We now have valid grey data
  m_GreyFileLoaded = 1;
  m_SegmentationLoaded = 0;

  m_InDrawingColor->set_changed();
  m_InDrawOverColor->set_changed();

  m_GlobalState->SetGreyExtension((char *)m_WizGreyIO->GetFileName());
  
  // Update the label of the UI
  UpdateMainLabel();

  // Get the largest image region
  GlobalState::RegionType roi = m_Driver->GetIRISImageData()->GetImageRegion();
  
  // Check if the region is real
  if (roi.GetNumberOfPixels() == 0) 
    {
    m_Activation->UpdateFlag( UIF_IRIS_ROI_VALID, false );
    m_GlobalState->SetIsValidROI(false);
    } 
  else 
    {
    m_Activation->UpdateFlag( UIF_IRIS_ROI_VALID, true );
    m_GlobalState->SetSegmentationROI(roi);
    m_GlobalState->SetIsValidROI(true);
    }   

  // Update the polygon buttons
  OnPolygonStateUpdate(0);
  OnPolygonStateUpdate(1);
  OnPolygonStateUpdate(2);

  // Update the image info window controls
  GreyImageWrapper *wrpGrey = m_Driver->GetCurrentImageData()->GetGrey();
  for(size_t d = 0; d < 3; d++)
    {
    m_OutImageInfoDimensions[d]->value(wrpGrey->GetSize()[d]);
    m_OutImageInfoOrigin[d]->value(wrpGrey->GetImage()->GetOrigin()[d]);
    m_OutImageInfoSpacing[d]->value(wrpGrey->GetImage()->GetSpacing()[d]);

    m_InImageInfoCursorIndex[d]->maximum(wrpGrey->GetSize()[d] - 1);
    m_InImageInfoCursorIndex[d]->minimum(0);
    m_InImageInfoCursorIndex[d]->step(1);
    
    m_OutImageInfoCursorPosition[d]->precision(2);
    m_OutImageInfoCursorNIFTIPosition[d]->precision(2);
    }
  m_OutImageInfoRange[0]->value(wrpGrey->GetImageMin());
  m_OutImageInfoRange[1]->value(wrpGrey->GetImageMax());

  // Set the RAI code in the image info dialog
  ImageCoordinateGeometry::DirectionMatrix dmat = 
    m_Driver->GetCurrentImageData()->GetImageGeometry().GetImageDirectionCosineMatrix();
  string raicode = 
    ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(dmat);
  string raitext;
  if(ImageCoordinateGeometry::IsDirectionMatrixOblique(dmat))
    {
    raitext = string("Oblique (closest to ") + raicode + string(")");
    }
  else
    raitext = raicode;  
  this->m_OutImageInfoOriginRAICode->value(raitext.c_str());

  // Now that we've loaded the image, check if there are any settings 
  // associated with it.  If there are, give the user an option to restore 
  // these settings
  Registry associated;
  if(m_SystemInterface->FindRegistryAssociatedWithFile(
    m_GlobalState->GetGreyFileName(),associated))
    {
    // Load the settings using RegistryIO
    SNAPRegistryIO rio;
    rio.ReadImageAssociatedSettings(associated,m_Driver,true,true,true,true);

    // Update the opacity slider
    m_InIRISLabelOpacity->value(m_GlobalState->GetSegmentationAlpha());

    // Update the polygon inversion state
    m_ChkInvertPolygon->value(m_GlobalState->GetPolygonInvert() ? 1 : 0);
    }
    
  // Redraw the crosshairs in the 3D window  
  m_IRISWindowManager3D->ResetView(); 

  // Redraw the intensity mapping window and controls
  m_IntensityCurveUI->SetCurve(m_Driver->GetIntensityCurve());    
  m_IntensityCurveUI->SetImageWrapper(
    m_Driver->GetCurrentImageData()->GetGrey());
  m_IntensityCurveUI->OnCurveChange();

  // Update the list of labels
  OnLabelListUpdate();

  // Warn if the image has been scaled
  GreyTypeToNativeFunctor native = 
    m_Driver->GetCurrentImageData()->GetGrey()->GetNativeMapping();
  if(m_AppearanceSettings->GetFlagFloatingPointWarningByDefault() == 1 && (native.scale != 1.0 || native.shift != 0.0))
    fl_alert(
      "The image you just loaded has a data type with greater range\n"
      "and precision than the 16-bit signed integer data type used \n"
      "internally by ITK-SNAP. Approximate intensity values will be\n"
      "reported by the program.\n\n"
      "CAUTION: if you use ITK-SNAP to save the image to a file, \n"
      "precision from the original image will be lost!\n\n"
	 "You can turn off this warning by going to \"Options -> \n"
	 "Display Options -> General\"\n\n");

  // Redraw the user interface
  RedrawWindows();
  m_WinMain->redraw();
}

void 
UserInterfaceLogic
::OnRGBImageUpdate()
{
  m_RGBFileLoaded = 1;

  // We need to reinitialize the slice windows
  OnImageGeometryUpdate();
  // for (unsigned int i=0; i<3; i++) 
  //  m_IRISWindowManager2D[i]->InitializeSlice(m_Driver->GetCurrentImageData());
  
  if(!m_Driver->GetCurrentImageData()->IsGreyLoaded())
    return;

  m_IRISWindowManager3D->ResetView();
  
  RedrawWindows();
  m_WinMain->redraw();
}

void 
UserInterfaceLogic
::OnSegmentationImageUpdate(bool reloaded)
{
  // Certain things required only if image is reloaded
  if(reloaded)
    {
    // TODO: what the heck is this variable?
    m_SegmentationLoaded = 1;

    // When an image is reloaded, we clear the previous undo points
    m_Driver->ClearUndoPoints();

    // The list of labels may have changed
    OnLabelListUpdate();

    // The 3D window needs to be reset
    m_IRISWindowManager3D->ResetView();
    }

  // Toggle the undo/redo flags
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());

  // The mesh is now dirty
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

  // Disable undo and redo 
  RedrawWindows();
  m_WinMain->redraw();
}

/* 
void 
UserInterfaceLogic
::OnSegmentationLabelsUpdate(bool resetCurrentAndDrawOverLabels)
{
  InitColorMap(resetCurrentAndDrawOverLabels);
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);
  
  m_WinMain->redraw();          
  m_OutMessage->value("Loading Label file successful");
}
*/

void 
UserInterfaceLogic
::OnSpeedImageUpdate()
{
  if(m_GlobalState->GetSpeedValid())
    {
    // Set UI state
    m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, true);

    // Choose to view the preprocessed image
    m_ChoiceSNAPView->value(m_MenuSNAPViewPreprocessed);
    // m_RadioSNAPViewPreprocessed->setonly();

    // Run the callback associated with that change
    OnViewPreprocessedSelect();
    }
  else
    {
    // Set UI state
    m_Activation->UpdateFlag(UIF_SNAP_SPEED_AVAILABLE, false);

    // Choose to view the grey image
    m_ChoiceSNAPView->value(m_MenuSNAPViewOriginal);
    // m_RadioSNAPViewOriginal->setonly();

    // Run the callback associated with that change
    OnSNAPViewOriginalSelect();
    }

  // Make sure color mapping is correct
  UpdateSpeedColorMap();
}

// This method should be called whenever the UI starts and every time that
// the history is updated.
void 
UserInterfaceLogic
::GenerateRecentFilesMenu()
{
  // Load the list of recent files from the history file
  const SystemInterface::HistoryListType &history = 
    m_SystemInterface->GetHistory("GreyImage");

  // Take the five most recent items and create menu items
  for(unsigned int i = 0; i < 5; i++)
    {
    // Get a pointer to the corresponding menu item
    Fl_Menu_Item *item = m_MenuLoadPreviousFirst + i;

    // Update it
    if( i < history.size()) 
      {
      // Populate each of the menu items
      m_RecentFileNames[i] = history[history.size() - (i+1)];
      item->label(m_RecentFileNames[i].c_str());
      item->activate();
      }
    else
      {
      m_RecentFileNames[i] = "Not Available";
      item->label(m_RecentFileNames[i].c_str());
      item->activate();
      }
    }

  // Enable / disable the overall menu
  if(history.size())
    m_MenuLoadPrevious->activate();
  else
    m_MenuLoadPrevious->deactivate();
}


void
UserInterfaceLogic
::OnLoadRecentAction(unsigned int iRecent)
{
  // Make sure the user doesn't lose any data
  if(!PromptBeforeLosingChanges(REASON_LOAD_MAIN)) return;

  // Get the history of grayscale images. Here we must be careful that every time
  // the history is updated, we also remember to update the recent files menu!!!
  const SystemInterface::HistoryListType &history = 
    m_SystemInterface->GetHistory("GreyImage");

  // Check that the history is OK
  if(history.size() <= iRecent)
    {
    fl_alert("Unable to load recent file due to internal error!");
    return;
    }

  // Get the recent file name
  string fnRecent = m_RecentFileNames[iRecent];

  // Show a wait cursor
  m_WinMain->cursor(FL_CURSOR_WAIT, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // TODO: At some point, we have to prompt the user that there are unsaved changes...
  try
    {
    // Load the file non-interactively
    this->NonInteractiveLoadGrey(fnRecent.c_str());

    // Restore the cursor
    m_WinMain->cursor(FL_CURSOR_DEFAULT, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
    }
  catch(itk::ExceptionObject &exc) 
    {
    // Restore the cursor
    m_WinMain->cursor(FL_CURSOR_DEFAULT, FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

    // Alert the user to the failure
    fl_alert("Error loading image:\n%s",exc.GetDescription());
    }
}

bool
UserInterfaceLogic
::PromptBeforeLosingChanges(PromptReason reason)
{
  // If there are unsaved changes, ask the user if they want to quit
  if(m_Activation->GetFlag(UIF_UNSAVED_CHANGES))
    {
    // Show the dialog
    m_WinConfirmDiscard->show();
    while(m_WinConfirmDiscard->shown())
      Fl::wait();

    // Check the value
    return m_ChkHiddenDiscardChanges->value() == 1;
    }
  else
    {
    return true;
    }
}

void 
UserInterfaceLogic
::NonInteractiveLoadGrey(const char *fname)
{
  // Perform the clean-up tasks before loading the image
  OnGreyImageUnload();

  // Load the image on the logical side
  m_Driver->LoadGreyImageFile(fname);

  // Update the system's history list
  m_SystemInterface->UpdateHistory("GreyImage", 
    itksys::SystemTools::CollapseFullPath(fname).c_str());

  // Update the list of recently open files
  GenerateRecentFilesMenu();

  // Save the filename
  m_GlobalState->SetGreyFileName(fname);
  m_GlobalState->SetSegmentationFileName("");

  // Update the user interface accordingly
  OnGreyImageUpdate();
}



void 
UserInterfaceLogic
::OnMenuLoadGrey() 
{
  // Make sure the user doesn't lose any data
if(!PromptBeforeLosingChanges(REASON_LOAD_MAIN)) return;

  // Set the history for the input wizard
  m_WizGreyIO->SetHistory(m_SystemInterface->GetHistory("GreyImage"));
  
  // Show the input wizard with no file selected
  m_WizGreyIO->DisplayInputWizard("");

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizGreyIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory("GreyImage", m_WizGreyIO->GetFileName());

    // Update the list of recently open files
    GenerateRecentFilesMenu();

    // Perform the clean-up tasks before loading the image
    OnGreyImageUnload();

    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISGreyImage(
      m_WizGreyIO->GetLoadedImage(), 
      GreyTypeToNativeFunctor(
        m_WizGreyIO->GetNativeScale(),
        m_WizGreyIO->GetNativeShift()));

    // Save the filename
    m_GlobalState->SetGreyFileName(m_WizGreyIO->GetFileName());
    m_GlobalState->SetSegmentationFileName("");

    // Update the user interface accordingly
    OnGreyImageUpdate();
    }
}

void 
UserInterfaceLogic
::NonInteractiveLoadRGBStandalone(const char *fname)
{
  // Perform the loading on the Logic side
  m_Driver->LoadRGBImageFile(fname);

  // Add the filename to the history
  m_SystemInterface->UpdateHistory("RGBImage",  
    itksys::SystemTools::CollapseFullPath(fname).c_str());

  // Set the RGB UI state      
  m_GlobalState->SetRGBAlpha(255);
  m_RGBOverlayUI->SetOpacity(255);

  // Update the user interface accordingly
  OnGreyImageUpdate();

  // Enable some menu items
  m_Activation->UpdateFlag(UIF_GRAY_LOADED, false);
  m_Activation->UpdateFlag(UIF_RGBBASE_LOADED, true);
  
  OnRGBImageUpdate();
}

void 
UserInterfaceLogic
::NonInteractiveLoadRGBOverlay(const char *fname)
{
  // Send the image and RAI to the IRIS application driver
  m_Driver->LoadRGBImageFile(fname);

  // Update the system's history list
  m_SystemInterface->UpdateHistory("RGBImage",  
    itksys::SystemTools::CollapseFullPath(fname).c_str());
  
  // Set the RGB UI state      
  m_GlobalState->SetRGBAlpha(128);
  m_RGBOverlayUI->SetOpacity(128);
  
  // Update the user interface accordingly
  OnRGBImageUpdate();

  // Set the state
  m_Activation->UpdateFlag(UIF_RGBOVL_LOADED, true); 
}

void 
UserInterfaceLogic
::OnMenuLoadRGB() 
{
  // Make sure the user doesn't lose any data
  if(!PromptBeforeLosingChanges(REASON_LOAD_MAIN)) return;

  // Set the history for the input wizard
  m_WizRGBIO->SetHistory(m_SystemInterface->GetHistory("RGBImage"));
  
  // Show the input wizard with no file selected
  m_WizRGBIO->DisplayInputWizard("");

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizRGBIO->IsImageLoaded()) 
    {
    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISRGBImage(m_WizRGBIO->GetLoadedImage());

    // Update the system's history list
    m_SystemInterface->UpdateHistory("RGBImage",m_WizRGBIO->GetFileName());

    // Save the filename
    m_GlobalState->SetRGBFileName(m_WizRGBIO->GetFileName());
    m_GlobalState->SetSegmentationFileName("");
    
    m_GlobalState->SetRGBAlpha(255);
    m_RGBOverlayUI->SetOpacity(255);
    
    // Update the user interface accordingly
    OnGreyImageUpdate();

    // Enable some menu items
    m_Activation->UpdateFlag(UIF_GRAY_LOADED, false);
    m_Activation->UpdateFlag(UIF_RGBBASE_LOADED, true);
    
    OnRGBImageUpdate();
    }
}

void 
UserInterfaceLogic
::OnMenuLoadRGBOverlay() 
{
  // Grey image should be loaded
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Should not be in SNAP mode
  assert(!m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizRGBOverlayIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Set the history for the input wizard
  m_WizRGBOverlayIO->SetHistory(
    m_SystemInterface->GetHistory("RGBImage"));
  
  // Show the input wizard
  m_WizRGBOverlayIO->DisplayInputWizard(
    m_GlobalState->GetRGBFileName());

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizRGBOverlayIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory("RGBImage",m_WizRGBOverlayIO->GetFileName());
    
    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISRGBImageOverlay(m_WizRGBOverlayIO->GetLoadedImage());

    // Save the filename
    m_GlobalState->SetRGBFileName(m_WizRGBOverlayIO->GetFileName());
    
    m_GlobalState->SetRGBAlpha(128);
    m_RGBOverlayUI->SetOpacity(128);
    
    // Update the user interface accordingly
    OnRGBImageUpdate();

    // Set the state
    m_Activation->UpdateFlag(UIF_RGBOVL_LOADED, true);
    }
  
  // Disconnect the input wizard from the grey image
  m_WizRGBOverlayIO->SetGreyImage(NULL);

  this->RedrawWindows();
}

void
UserInterfaceLogic
::NonInteractiveLoadLabels(const char *file)
{
  // Read the labels from file
  m_Driver->GetColorLabelTable()->LoadFromFile(file);

  // Reset the current drawing and overlay labels
  m_GlobalState->SetDrawingColorLabel(
    m_Driver->GetColorLabelTable()->GetFirstValidLabel());
  m_GlobalState->SetOverWriteColorLabel(0);
  
  // Update the label editor window
  m_LabelEditorUI->OnLabelListUpdate(
    m_GlobalState->GetDrawingColorLabel());

  // Update the user interface in response
  OnLabelListUpdate();

  // Update the history
  m_SystemInterface->UpdateHistory("LabelDescriptions", 
    itksys::SystemTools::CollapseFullPath(file).c_str());  
}


void 
UserInterfaceLogic
::OnLoadLabelsAction() 
{
  // Try reading the file
  try 
    {
    // Read the labels from file
    this->NonInteractiveLoadLabels(m_DlgLabelsIO->GetFileName());
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error loading labels:\n%s", exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnSaveLabelsAction()
{
  // Get the selected file name
  const char *file = m_DlgLabelsIO->GetFileName();

  // Try writing
  try 
    {
    // Write labels to the file
    m_Driver->GetColorLabelTable()->SaveToFile(file);
    
    // Update the history
    m_SystemInterface->UpdateHistory("LabelDescriptions",file);
    }
  catch (itk::ExceptionObject &exc) 
    {
    // Alert the user to the failure
    fl_alert("Error writing labels:\n%s",exc.GetDescription());

    // Rethrow the exception
    throw;
    }
}

void 
UserInterfaceLogic
::OnMenuLoadLabels() 
{
  // Display the load labels dialog
  m_DlgLabelsIO->SetTitle("Load Labels");
  m_DlgLabelsIO->DisplayLoadDialog(
    m_SystemInterface->GetHistory("LabelDescriptions"));
}

void UserInterfaceLogic
::OnMenuSaveLabels() 
{
  // Display the save labels dialog
  m_DlgLabelsIO->SetTitle("Save Labels");
  m_DlgLabelsIO->DisplaySaveDialog(
    m_SystemInterface->GetHistory("LabelDescriptions"));
}

void
UserInterfaceLogic
::OnActiveWindowSaveSnapshot(unsigned int window)
{
  // Generate a filename for the screenshot
  std::string finput = GenerateScreenShotFilename();
  
  // Create a file chooser
  std::string file = itksys::SystemTools::GetFilenameName(finput);
  std::string path = itksys::SystemTools::GetFilenamePath(finput);

  // Store the current directory
  std::string dir = itksys::SystemTools::GetCurrentWorkingDirectory();
  itksys::SystemTools::ChangeDirectory(path.c_str());

  // Get the filename 
  const char *fchosen = 
    fl_file_chooser("Save PNG Snapshot", "PNG Files (*.png)", file.c_str());
  
  // Restore the current directory
  itksys::SystemTools::ChangeDirectory(dir.c_str());

  if(!fchosen)
    return;

  // Store the filename for incrementing numerical names
  m_LastSnapshotFileName = fchosen;

  // Check if the user omitted the extension
  if(itksys::SystemTools::GetFilenameExtension(m_LastSnapshotFileName) == "")
    m_LastSnapshotFileName = m_LastSnapshotFileName + ".png";

  // Choose which window to save with
  if(window < 3)
    m_SliceWindow[window]->SaveAsPNG(m_LastSnapshotFileName.c_str());
  else
    m_RenderWindow->SaveAsPNG(m_LastSnapshotFileName.c_str());
}

void UserInterfaceLogic
::OnMenuSaveScreenshot(unsigned int iSlice)
{
  size_t iWindow = 
    m_Driver->GetDisplayWindowForAnatomicalDirection(
      (AnatomicalDirection) iSlice);
  OnActiveWindowSaveSnapshot(iWindow);
}

void 
UserInterfaceLogic
::OnMenuSaveScreenshotSeries(unsigned int iSlice)
{
  // iSlice needs to be between 0 and 2
  assert (iSlice >= 0 && iSlice <= 2);

  // iSlice refers to which anatomical direction the user wants to
  // animate along (0 = axial, 1 = sagittal, 2 = coronal). We need
  // to know which window that corresponds to, as well as what image
  // dimension

  // Find the display window corresponding to this anatomical direction
  size_t iWindow = 
    m_Driver->GetDisplayWindowForAnatomicalDirection(
      (AnatomicalDirection) iSlice);

  // Get the image slicing direction
  size_t iImageDir = 
    m_Driver->GetImageDirectionForAnatomicalDirection(
      (AnatomicalDirection) iSlice);

  // let the user pick the directory for saving the screenshots
  char *path = fl_dir_chooser(
    "Select the directory to save the screenshots", NULL, 0);
  
  // set up the 1st snapshot name
  std::string fname;
  if (path)
  {
    fname = path;
  } else
  {
    return;
  }
  
  switch (iSlice)
  {
    default:
    case 0: fname += "axial0001.png";
		  break;
    case 1: fname += "saggital0001.png";
		  break;
    case 2: fname += "coronal0001.png";
		  break;
  }
  
  // back up cursor location
  Vector3ui xCrossImageOld = m_Driver->GetCursorPosition();
  Vector3ui xCrossImage = xCrossImageOld;
  Vector3ui xSize = m_Driver->GetCurrentImageData()->GetVolumeExtents();
  xCrossImage[iImageDir] = 0;
  
  // turn sync off temporarily
  unsigned int syncValue = m_BtnSynchronizeCursor->value();
  m_BtnSynchronizeCursor->value(0);

  for (size_t i = 0; i < xSize[iImageDir]; ++i)
  {
    m_Driver->SetCursorPosition(xCrossImage);
    this->OnCrosshairPositionUpdate();
    this->RedrawWindows();
    m_SliceWindow[iWindow]->SaveAsPNG(fname.c_str());
    xCrossImage[iImageDir]++;
    m_LastSnapshotFileName = fname;
    fname = GenerateScreenShotFilename();
  }
  
  // recover the original cursor position
  m_Driver->SetCursorPosition(xCrossImageOld);
  this->OnCrosshairPositionUpdate();
  this->RedrawWindows();  
  
  // turn sync back on
  m_BtnSynchronizeCursor->value(syncValue);
}

void UserInterfaceLogic
::OnMenuExportSlice(unsigned int iSlice)
{
  // We need to get a filename for the export
  char* fName = fl_file_chooser(
    "Export Slice As", "Image Files (*.{png,jpg,gif,tiff})", NULL);

  if(fName)
    m_Driver->ExportSlice((AnatomicalDirection) iSlice, fName);
}

void UserInterfaceLogic
::OnSynchronizeCursorAction()
{
  if(m_BtnSynchronizeCursor->value())
    {
    // Send the cursor position to the rest of the world
    this->OnCrosshairPositionUpdate();
    }
}

void UserInterfaceLogic
::OnMenuNewSegmentation()
{
  // Grey image should be loaded
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Should not be in SNAP mode
  assert(!m_GlobalState->GetSNAPActive());

  // Prompt to save changes
  if(!PromptBeforeLosingChanges(REASON_LOAD_SEGMENTATION)) return;

  // Get the driver to clear the image
  m_Driver->ClearIRISSegmentationImage();

  // There are now no unsaved changes
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, false);

  // Clear the segmentation file name
  m_GlobalState->SetSegmentationFileName("");

  // Update the main window label
  this->UpdateMainLabel();

  // Save the state of the Undo manager at the time the image was updated
  m_UndoStateAtLastIO = m_Driver->GetUndoManager().GetState();

  // Update the user interface
  OnSegmentationImageUpdate(true);
}


void UserInterfaceLogic
::NonInteractiveLoadSegmentation(const char *fname)
{
  // Send the image and RAI to the IRIS application driver
  m_Driver->LoadLabelImageFile(fname);

  // Update the system's history list
  m_SystemInterface->UpdateHistory("SegmentationImage",  
    itksys::SystemTools::CollapseFullPath(fname).c_str());

  // There are now no unsaved changes
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, false);

  // Update main window label
  this->UpdateMainLabel();

  // Save the state of the Undo manager at the time the image was updated
  m_UndoStateAtLastIO = m_Driver->GetUndoManager().GetState(); 

  // The mesh has become dirty
  this->OnSegmentationImageUpdate(true);
}

void UserInterfaceLogic
::OnMenuLoadSegmentation() 
{
  // Grey image should be loaded
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Should not be in SNAP mode
  assert(!m_GlobalState->GetSNAPActive());

  // Prompt to save changes
  if(!PromptBeforeLosingChanges(REASON_LOAD_SEGMENTATION)) return;

  // Set up the input wizard with the grey image
  m_WizSegmentationIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Set the history for the input wizard
  m_WizSegmentationIO->SetHistory(
    m_SystemInterface->GetHistory("SegmentationImage"));
  
  // Show the input wizard
  m_WizSegmentationIO->DisplayInputWizard(
    m_GlobalState->GetLastAssociatedSegmentationFileName());

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizSegmentationIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory(
      "SegmentationImage",m_WizSegmentationIO->GetFileName());

    // Send the image and RAI to the IRIS application driver
    m_Driver->UpdateIRISSegmentationImage(m_WizSegmentationIO->GetLoadedImage());

    // Save the segmentation file name
    m_GlobalState->SetSegmentationFileName(m_WizSegmentationIO->GetFileName());
    m_GlobalState->SetLastAssociatedSegmentationFileName(m_WizSegmentationIO->GetFileName());   

    // There are now no unsaved changes
    m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, false);

    // Update the label of the main window
    this->UpdateMainLabel();

    // Save the state of the Undo manager at the time the image was updated
    m_UndoStateAtLastIO = m_Driver->GetUndoManager().GetState();

    // Segmentation has been updated
    this->OnSegmentationImageUpdate(true);
    }

  // Disconnect the input wizard from the grey image
  m_WizSegmentationIO->SetGreyImage(NULL);
}

void UserInterfaceLogic
::OnMenuSaveGrey()
{
  // Set the history for the wizard
  m_WizGreyIO->SetHistory(m_SystemInterface->GetHistory("GreyImage"));

  // Save the segmentation
  if(m_WizGreyIO->DisplaySaveWizard(
    m_Driver->GetIRISImageData()->GetGrey()->GetImage(), NULL))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "GreyImage", m_WizGreyIO->GetSaveFileName());
    }
}

void UserInterfaceLogic
::OnMenuSaveGreyROI() 
{
  // Better be in snap
  assert(m_GlobalState->GetSNAPActive());

  // Set the history for the wizard
  m_WizGreyIO->SetHistory(m_SystemInterface->GetHistory("GreyImage"));

  // Save the segmentation
  if(m_WizGreyIO->DisplaySaveWizard(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage(),NULL))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "GreyImage",m_WizGreyIO->GetSaveFileName());
    }
}

void UserInterfaceLogic
::OnMenuSaveSegmentation() 
{
  // Better have a segmentation image
  assert(m_Driver->GetIRISImageData()->IsSegmentationLoaded());

  // Set the history for the wizard
  m_WizSegmentationIO->SetHistory(
    m_SystemInterface->GetHistory("SegmentationImage"));

  // Save the segmentation
  if(m_WizSegmentationIO->DisplaySaveWizard(
    m_Driver->GetIRISImageData()->GetSegmentation()->GetImage(),
    m_GlobalState->GetLastAssociatedSegmentationFileName()))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "SegmentationImage",m_WizSegmentationIO->GetSaveFileName());

    // Store the new filename
    m_GlobalState->SetSegmentationFileName(
      m_WizSegmentationIO->GetSaveFileName());
    m_GlobalState->SetLastAssociatedSegmentationFileName(
      m_WizSegmentationIO->GetSaveFileName());

    // Save the state of the undo manager
    m_UndoStateAtLastIO = m_Driver->GetUndoManager().GetState();

    // The segmentation is no longer dirty
    m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, false);

    }
}

void UserInterfaceLogic
::OnMenuSaveSegmentationMesh() 
{
  // Better have a segmentation image
  assert(m_Driver->GetIRISImageData()->IsSegmentationLoaded());

  // Send the history information to the wizard
  m_WizMeshExport->SetHistory(
    m_SystemInterface->GetHistory("SegmentationMesh"));

  // Display the segmentation wizard
  if(m_WizMeshExport->DisplayWizard(m_Driver))
    {
    // Get the save settings
    MeshExportSettings sets = m_WizMeshExport->GetExportSettings();

    // Use the settings to save the mesh
    m_Driver->ExportSegmentationMesh(sets, m_ProgressCommand);

    // Update the history
    m_SystemInterface->UpdateHistory("SegmentationMesh", sets.GetMeshFileName().c_str());
    }
}

void 
UserInterfaceLogic
::OnMenuLoadPreprocessed() 
{
  this->OnLoadPreprocessedImageAction();
}

void
UserInterfaceLogic
::OnMenuLoadAdvection()
{
  typedef itk::Image<float, 3> AdvectionImage;
  AdvectionImage::Pointer imgAdvection[3];
  
  // Preprocessing image can only be loaded in SNAP mode
  assert(m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizPreprocessingIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Load the three advection images as floating point
  bool flagLoadCompleted = true;
  for(unsigned int i=0;i<3;i++)
    {
    // Set the history for the input wizard
    m_WizPreprocessingIO->SetHistory(
      m_SystemInterface->GetHistory("AdvectionImage"));

    // Show the input wizard
    m_WizPreprocessingIO->DisplayInputWizard(
      m_GlobalState->GetAdvectionFileName(i));

    // If the load operation was successful, populate the data and GUI with the
    // new image
    if(m_WizPreprocessingIO->IsImageLoaded()) 
      {
      // Update the system's history list
      m_SystemInterface->UpdateHistory(
        "AdvectionImage",m_WizPreprocessingIO->GetFileName());

      // Update the application with the new speed image
      imgAdvection[i] = m_WizPreprocessingIO->GetLoadedImage();

      // Save the segmentation file name
      m_GlobalState->SetAdvectionFileName(i, m_WizPreprocessingIO->GetFileName());
      }
    else
      {
      flagLoadCompleted = false;
      break;
      }
    }
  
  // Disconnect the input wizard from the grey image
  m_WizPreprocessingIO->SetGreyImage(NULL);

  // Add the advection image to SNAP
  if(flagLoadCompleted)
    m_Driver->GetSNAPImageData()->SetExternalAdvectionField(
      imgAdvection[0],imgAdvection[1],imgAdvection[2]);
}

void 
UserInterfaceLogic
::OnLoadPreprocessedImageAction() 
{
  // Preprocessing image can only be loaded in SNAP mode
  assert(m_GlobalState->GetSNAPActive());
  
  // Set up the input wizard with the grey image
  m_WizPreprocessingIO->SetGreyImage(
    m_Driver->GetCurrentImageData()->GetGrey()->GetImage());

  // Set the history for the input wizard
  m_WizPreprocessingIO->SetHistory(
    m_SystemInterface->GetHistory("PreprocessingImage"));

  // Show the input wizard
  m_WizPreprocessingIO->DisplayInputWizard(
    m_GlobalState->GetLastAssociatedPreprocessingFileName());

  // If the load operation was successful, populate the data and GUI with the
  // new image
  if(m_WizPreprocessingIO->IsImageLoaded()) 
    {
    // Update the system's history list
    m_SystemInterface->UpdateHistory(
      "PreprocessingImage",m_WizPreprocessingIO->GetFileName());

    // Update the application with the new speed image
    m_Driver->UpdateSNAPSpeedImage(
      m_WizPreprocessingIO->GetLoadedImage(),
      m_RadSnakeEdge->value() ? EDGE_SNAKE : IN_OUT_SNAKE);
    
    // Save the segmentation file name
    m_GlobalState->SetPreprocessingFileName(m_WizPreprocessingIO->GetFileName());
    m_GlobalState->SetLastAssociatedPreprocessingFileName(m_WizPreprocessingIO->GetFileName());

    // Update the UI flag
    m_Activation->UpdateFlag(UIF_SNAP_PREPROCESSING_DONE, true);

    // Update the user interface accordingly
    OnSpeedImageUpdate();
    }

  // Disconnect the input wizard from the grey image
  m_WizPreprocessingIO->SetGreyImage(NULL);
}

void 
UserInterfaceLogic
::OnMenuSavePreprocessed() 
{
  // Better have a speed image to save
  assert(m_GlobalState->GetSpeedValid());

  // Set the history for the wizard
  m_WizPreprocessingIO->SetHistory(
    m_SystemInterface->GetHistory("PreprocessingImage"));

  // Save the speed
  if(m_WizPreprocessingIO->DisplaySaveWizard(
    m_Driver->GetSNAPImageData()->GetSpeed()->GetImage(),
    m_GlobalState->GetLastAssociatedPreprocessingFileName()))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "PreprocessingImage",m_WizPreprocessingIO->GetSaveFileName());
    
    // Store the new filename
    m_GlobalState->SetPreprocessingFileName(
      m_WizPreprocessingIO->GetSaveFileName());
    m_GlobalState->SetLastAssociatedPreprocessingFileName(
      m_WizPreprocessingIO->GetSaveFileName());
    }
}

void 
UserInterfaceLogic
::OnMenuSaveLevelSet() 
{
  // Better have a snake image to save
  assert(m_Driver->GetSNAPImageData()->IsSnakeLoaded());

  // Set the history for the wizard
  m_WizLevelSetIO->SetHistory(
    m_SystemInterface->GetHistory("LevelSetImage"));

  // Save the speed
  if(m_WizLevelSetIO->DisplaySaveWizard(
    m_Driver->GetSNAPImageData()->GetSnake()->GetImage(),
    m_GlobalState->GetLevelSetFileName()))
    {
    // Update the history for the wizard
    m_SystemInterface->UpdateHistory(
      "LevelSetImage", m_WizLevelSetIO->GetSaveFileName());
    
    // Store the new filename
    m_GlobalState->SetLevelSetFileName(
      m_WizPreprocessingIO->GetSaveFileName());
    }
}


void UserInterfaceLogic
::OnMenuIntensityCurve() 
{
  // The image should be loaded before bringing up the curve
  assert(m_Driver->GetCurrentImageData()->IsGreyLoaded());

  // Update the curve in the UI dialog (is this necessary?)
  // m_IntensityCurveUI->SetCurve(m_Driver->GetIntensityCurve());    
  // m_IntensityCurveUI->SetImageWrapper(
  //  m_Driver->GetCurrentImageData()->GetGrey());

  // Show the window
  m_IntensityCurveUI->DisplayWindow();
}

void UserInterfaceLogic
::OnIntensityCurveUpdate() 
{
  // Update the image wrapper
  m_Driver->GetCurrentImageData()->GetGrey()->SetIntensityMapFunction(
    m_Driver->GetIntensityCurve());

  // Update the display
  RedrawWindows();
}

void UserInterfaceLogic
::OnRGBOverlayOptionsUpdate()
{
  m_GlobalState->SetRGBAlpha(m_RGBOverlayUI->GetOpacity());
  this->RedrawWindows();
}

void
UserInterfaceLogic
::OnIRISLabelOpacityChange()
{
  m_GlobalState->SetSegmentationAlpha( (unsigned char) m_InIRISLabelOpacity->value());  
  this->RedrawWindows();
}

void
UserInterfaceLogic
::OnSNAPLabelOpacityChange()
{
  m_GlobalState->SetSegmentationAlpha( (unsigned char) m_InSNAPLabelOpacity->value());  
  this->RedrawWindows();
}

void
UserInterfaceLogic
::OnLaunchTutorialAction()
{
  // Find the tutorial file name
  ShowHTMLPage("Tutorial.html");
}

void
UserInterfaceLogic
::ShowHTMLPage(const char *link)
{
  // Get the path to the file name
  string completeLink = string("HTMLHelp/") +  link;
  string file = 
    m_SystemInterface->GetFileInRootDirectory(completeLink.c_str());

  // Show the help window
  m_HelpUI->ShowHelp(file.c_str());  
}

void
UserInterfaceLogic
::ShowSplashScreen()
{
  // Place the window in the center of display
  CenterChildWindowInMainWindow(m_WinSplash);

  // Show the splash screen
  m_WinSplash->show();

  // Show a wait cursor
  m_WinSplash->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  m_WinMain->cursor(FL_CURSOR_WAIT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);

  // Make FL update the screen
  Fl::check();

  // Save the time of the splash screen
  m_SplashScreenStartTime = clock();
}

void
UserInterfaceLogic
::HideSplashScreen()
{
  // Wait a second with the splash screen
  while(clock() - m_SplashScreenStartTime < CLOCKS_PER_SEC) {}

  // Clear the cursor
  m_WinMain->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  m_WinSplash->cursor(FL_CURSOR_DEFAULT,FL_FOREGROUND_COLOR, FL_BACKGROUND_COLOR);
  
  // Hide the screen 
  m_WinSplash->hide();
}

void
UserInterfaceLogic
::UpdateSplashScreen(const char *message)
{
  m_OutSplashMessage->label(message);
  Fl::check();

  // Save the time of the splash screen
  m_SplashScreenStartTime = clock();
}

void
UserInterfaceLogic
::CenterChildWindowInParentWindow(Fl_Window *childWindow,
                                  Fl_Window *parentWindow)
{
  int px = parentWindow->x() + (parentWindow->w() - childWindow->w()) / 2;
  int py = parentWindow->y() + (parentWindow->h() - childWindow->h()) / 2;
  childWindow->resize(px,py,childWindow->w(),childWindow->h());
}

void 
UserInterfaceLogic
::CenterChildWindowInMainWindow(Fl_Window *childWindow)
{
  CenterChildWindowInParentWindow(childWindow,m_WinMain);
}

void
UserInterfaceLogic
::OnResetView2DAction(unsigned int window)
{
  m_SliceCoordinator->ResetViewToFitInOneWindow(window);
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnResetAllViews2DAction()
{
  m_SliceCoordinator->ResetViewToFitInAllWindows();
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnLinkedZoomChange()
{
  if(m_ChkLinkedZoom->value() > 0)
    {
    m_SliceCoordinator->SetLinkedZoom(true);
    m_Activation->UpdateFlag(UIF_LINKED_ZOOM, true);
    }
  else
    {
    m_ChkMultisessionZoom->value(0);
    m_SliceCoordinator->SetLinkedZoom(false);
    m_Activation->UpdateFlag(UIF_LINKED_ZOOM, false);
    }
  
  // Update the zoom level display
  OnZoomUpdate();
}

void
UserInterfaceLogic
::OnMultisessionZoomChange()
{
  // Make sure we broadcast the current zoom level
  this->OnZoomUpdate();
}

void
UserInterfaceLogic
::OnZoomLevelChange()
{
  float zoom = m_InZoomLevel->value();
  m_SliceCoordinator->SetZoomLevelAllWindows(zoom);
  OnZoomUpdate();
}

void 
UserInterfaceLogic
::OnZoomUpdate(bool flagBroadcast)
{
  // This method should be called whenever the zoom changes
  if(m_SliceCoordinator->GetLinkedZoom())
    {
    // Get the zoom from the first window and display it on the screen
    m_InZoomLevel->value(m_SliceCoordinator->GetCommonZoomLevel());

    // Broadcast the zoom level to other sessions
    if(flagBroadcast 
      && m_ChkMultisessionZoom->value()
      && m_Activation->GetFlag(UIF_IRIS_ACTIVE))
      {
      m_SystemInterface->IPCBroadcastZoomLevel(
        m_SliceCoordinator->GetCommonZoomLevel());
      }
    }

  else
    {
    // Otherwise, clear the display
    m_InZoomLevel->value(0);
    }
}


// Get the window under mouse focus or -1 if none
int 
UserInterfaceLogic
::GetWindowUnderFocus(void)
{
  for(int i = 0; i < 3; i++)
    if(m_SliceCoordinator->GetWindow(i)->GetCanvas()->GetFocus())
      return i;
  return -1;
}

std::string
UserInterfaceLogic
::GenerateScreenShotFilename()
{
  // Get the last screen shot filename used
  std::string last = m_LastSnapshotFileName;
  if(last.length() == 0)
    return "snapshot0001.png";

  // Count how many digits there are at the end of the filename
  std::string noext = 
    itksys::SystemTools::GetFilenameWithoutExtension(m_LastSnapshotFileName);
  unsigned int digits = 0;
  for(int i = noext.length() - 1; i >= 0; i--)
    if(isdigit(noext[i])) digits++; else break;

  // If there are no digits, return the filename
  if(digits == 0) return m_LastSnapshotFileName;

  // Get the number at the end of the string
  std::string snum = noext.substr(noext.length() - digits);
  std::istringstream iss(snum);
  unsigned long num = 0;
  iss >> num;

  // Increment the number by one and convert to another string, padding with zeros
  std::ostringstream oss;
  oss << itksys::SystemTools::GetFilenamePath(last);
  oss << "/";
  oss << noext.substr(0, noext.length() - digits);
  oss << std::setw(digits) << std::setfill('0') << (num + 1);
  oss << itksys::SystemTools::GetFilenameExtension(m_LastSnapshotFileName);
  return oss.str();
}



void
UserInterfaceLogic
::OnUndoAction()
{
  m_Driver->Undo();

  // Update the flags in the UI
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

  // Update the saved changes flag (now true, unless equal to saved image)
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, 
    m_UndoStateAtLastIO != m_Driver->GetUndoManager().GetState());

  RedrawWindows();
}

void
UserInterfaceLogic
::OnRedoAction()
{
  m_Driver->Redo();

  // Update the flags in the UI
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());
  m_Activation->UpdateFlag(UIF_IRIS_MESH_DIRTY, true);

  // Update the saved changes flag (now true, unless equal to saved image)
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, 
    m_UndoStateAtLastIO != m_Driver->GetUndoManager().GetState());

  RedrawWindows();
}

void
UserInterfaceLogic
::StoreUndoPoint(const char *text)
{
  // the actual undo point is handled in IRISApplication
  m_Driver->StoreUndoPoint(text);

  // Update the flags in the UI
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());

  // Update the saved changes flag (now true, unless equal to saved image)
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, 
    m_UndoStateAtLastIO != m_Driver->GetUndoManager().GetState());
}

void
UserInterfaceLogic
::ClearUndoPoints()
{
  m_Driver->ClearUndoPoints();

  // Update the flags in the UI
  m_Activation->UpdateFlag(UIF_UNDO_POSSIBLE, m_Driver->IsUndoPossible());
  m_Activation->UpdateFlag(UIF_REDO_POSSIBLE, m_Driver->IsRedoPossible());

  // Update the saved changes flag (now true, unless equal to saved image)
  m_Activation->UpdateFlag(UIF_UNSAVED_CHANGES, 
    m_UndoStateAtLastIO != m_Driver->GetUndoManager().GetState());
}

void 
UserInterfaceLogic
::OnUnsavedChangesStateChange(UIStateFlags flag, bool value)
{
  this->UpdateMainLabel();
}

/*
 *$Log: UserInterfaceLogic.cxx,v $
 *Revision 1.36  2009/01/22 23:14:10  garyhuizhang
 *1) Add an option under Options -> Display Options -> General for toggling on/off the warning when loading floating point images.
 *2) The warning message is modified to include the instruction for turning it off.
 *
 *Revision 1.35  2009/01/17 10:40:28  pyushkevich
 *Added synchronization to 3D window viewpoint
 *
 *Revision 1.34  2009/01/16 21:31:41  pyushkevich
 *Fixed issues with loading and creating new segmentation images; namely undo/redo bugs and update mesh button not being available.
 *
 *Revision 1.33  2008/12/02 21:43:24  pyushkevich
 *Reorganization of the watershed code
 *
 *Revision 1.32  2008/12/02 05:14:19  pyushkevich
 *New feature: watershed-based adaptive paint brush. Based on the similar tool in ITK-Grey (which was derived from ITK-SNAP).
 *
 *Revision 1.31  2008/11/20 04:24:00  pyushkevich
 *Bugfixes
 *
 *Revision 1.30  2008/11/20 02:41:03  pyushkevich
 *Now when non-native format is loaded, the intensity is mapped to original values
 *
 *Revision 1.29  2008/11/17 19:47:41  pyushkevich
 *Get linux to compile
 *
 *Revision 1.28  2008/11/17 19:38:23  pyushkevich
 *Added tools dialog to label editor window
 *
 *Revision 1.26  2008/11/01 11:32:00  pyushkevich
 *Compatibility with ITK 3.8 support for reading oriented images
 *Command line loading of RGB images
 *Improved load-image commands in UserInterfaceLogic
 *
 *Revision 1.25  2008/04/16 13:48:01  pyushkevich
 *Major bug fix: cursor was not working in automatic segmentation mode
 *
 *Revision 1.24  2008/04/01 14:25:27  pyushkevich
 *Minor bug fix: alt-i to do automatic intensity adjustment
 *
 *Revision 1.23  2008/03/25 19:31:31  pyushkevich
 *Bug fixes for release 1.6.0
 *
 *Revision 1.22  2008/02/27 04:34:46  garyhuizhang
 *1) rename OnMenuSaveScreenshots to OnMenuSaveScreenshotSeries
 *2) support menu access to both save single screenshot and screenshot series
 *
 *Revision 1.21  2008/02/26 21:28:29  garyhuizhang
 *improve the behavior of savescreenshot series
 *1) restore the cursor location before the call
 *2) handle the situation when user did not provide a directory for screenshots
 *
 *Revision 1.20  2008/02/23 23:41:12  garyhuizhang
 *add support for saving screenshots of the whole image volume
 *
 *Revision 1.19  2008/02/16 22:38:34  pyushkevich
 *Bug fix with bubbles
 *
 *Revision 1.18  2008/02/15 19:55:31  pyushkevich
 *fixed 100% cpu usage bug (from idle function)
 *
 *Revision 1.17  2008/02/15 18:34:16  pyushkevich
 *scrolling bug fix; packaging includes date in filename
 *
 *Revision 1.16  2008/02/11 17:49:20  pyushkevich
 *Touchups
 *
 *Revision 1.15  2008/02/11 12:59:40  pyushkevich
 *bug fix with shared cursor on Linux
 *
 *Revision 1.14  2008/02/10 23:55:22  pyushkevich
 *Added "Auto" button to the intensity curve window; Added prompt before quitting on unsaved data; Fixed issues with undo on segmentation image load; Added synchronization between SNAP sessions.
 *
 *Revision 1.13  2008/01/08 20:34:52  pyushkevich
 *Implement toggle for opacity slider
 *
 *Revision 1.12  2007/12/30 04:05:18  pyushkevich
 *GPL License
 *
 *Revision 1.11  2007/12/25 15:46:23  pyushkevich
 *Added undo/redo functionality to itk-snap
 *
 *Revision 1.10  2007/09/18 18:42:40  pyushkevich
 *Added tablet drawing to polygon mode
 *
 *Revision 1.9  2007/09/15 15:59:20  pyushkevich
 *Improved the paintbrush mode, allowed more variety of brush sizes
 *
 *Revision 1.8  2007/09/04 16:56:13  pyushkevich
 *tablet support 1
 *
 *Revision 1.7  2007/06/07 00:49:16  pyushkevich
 *Debugged RGB changes
 *
 *Revision 1.6  2007/06/06 22:27:22  garyhuizhang
 *Added support for RGB images in SNAP
 *
 *Revision 1.5  2007/05/10 20:19:50  pyushkevich
 *Added VTK mesh export code and GUI
 *
 *Revision 1.4  2006/12/06 14:36:18  pyushkevich
 *Fixes for VC6
 *
 *Revision 1.3  2006/12/06 13:27:46  pyushkevich
 *Followup checking for 1.4.1
 *
 *Revision 1.2  2006/12/06 01:26:07  pyushkevich
 *Preparing for 1.4.1. Seems to be stable in Windows but some bugs might be still there
 *
 *Revision 1.1  2006/12/02 04:22:23  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.59  2006/02/02 01:23:10  pauly
 *BUG: Fixed SNAP bugs in the last checkin
 *
 *Revision 1.58  2006/02/01 21:41:42  pauly
 *ENH: SNAP: bubble radius changed to floating point
 *
 *Revision 1.57  2006/02/01 20:21:26  pauly
 *ENH: An improvement to the main SNAP UI structure: one set of GL windows is used to support SNAP and IRIS modes
 *
 *Revision 1.56  2006/01/06 18:45:35  pauly
 *BUG: Progress bar in SNAP not disappearing at times
 *
 *Revision 1.55  2006/01/05 23:19:45  pauly
 *BUG: SNAP label file was not being written correctly
 *
 *Revision 1.54  2006/01/05 18:25:05  pauly
 *BUG: Fixed cursor bug in lInux
 *
 *Revision 1.53  2006/01/05 00:02:41  pauly
 *ENH: Now SNAP keeps cursor position whether you enter or exit auto mode
 *
 *Revision 1.52  2006/01/04 23:25:42  pauly
 *ENH: SNAP will keep crosshairs position when entering automatic segmentation
 *
 *Revision 1.51  2005/12/21 17:07:22  pauly
 *STYLE: New SNAP logo, about window
 *
 *Revision 1.50  2005/12/19 03:43:12  pauly
 *ENH: SNAP enhancements and bug fixes for 1.4 release
 *
 *Revision 1.49  2005/12/16 23:46:51  pauly
 *BUG: Silly mistake in PNG screenshot saver
 *
 *Revision 1.48  2005/12/12 13:11:39  pauly
 *BUG: Filename problem with talking snapshots in SNAP fixed
 *
 *Revision 1.47  2005/12/12 00:27:44  pauly
 *ENH: Preparing SNAP for 1.4 release. Snapshot functionality
 *
 *Revision 1.46  2005/12/08 21:15:58  pauly
 *COMP: SNAP not linking because whoever did previous fix did not check in SNAPCommon.cxx
 *
 *Revision 1.45  2005/12/08 18:20:46  hjohnson
 *COMP:  Removed compiler warnings from SGI/linux/MacOSX compilers.
 *
 *Revision 1.44  2005/11/23 14:32:15  ibanez
 *BUG: 2404. Patch provided by Paul Yushkevish.
 *
 *Revision 1.43  2005/11/10 23:02:14  pauly
 *ENH: Added support for VoxBo CUB files to ITK-SNAP, as well as some cosmetic touches
 *
 *Revision 1.42  2005/11/03 20:59:15  pauly
 *COMP: Fixed compiler errors on newer versions of VTK
 *
 *Revision 1.41  2005/11/03 18:45:29  pauly
 *ENH: Enabled SNAP to read DICOM Series
 *
 *Revision 1.40  2005/10/29 14:00:15  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.39  2005/08/10 19:57:15  pauly
 *BUG: Labels not always appearing when loading an image in SNAP
 *
 *Revision 1.38  2005/08/10 03:24:20  pauly
 *BUG: Corrected problems with 3D window, label IO from association files
 *
 *Revision 1.37  2005/04/23 13:58:19  pauly
 *COMP: Fixing compile errors in the last SNAP checkin
 *
 *Revision 1.36  2005/04/23 12:56:59  lorensen
 *BUG: bad include.
 *
 *Revision 1.35  2005/04/21 18:52:38  pauly
 *ENH: Furhter improvements to SNAP label editor
 *
 *Revision 1.34  2005/04/21 14:46:30  pauly
 *ENH: Improved management and editing of color labels in SNAP
 *
 *Revision 1.33  2005/04/15 19:04:19  pauly
 *ENH: Improved the Intensity Contrast features in SNAP
 *
 *Revision 1.32  2005/04/14 16:35:10  pauly
 *ENH: Added Image Info window to SNAP
 *
 *Revision 1.31  2005/03/08 03:12:51  pauly
 *BUG: Minor bugfixes in SNAP, mostly to the user interface
 *
 *Revision 1.30  2004/09/21 15:50:40  jjomier
 *FIX: vector_multiply_mixed requires template parameters otherwise MSVC cannot deduce them
 *
 *Revision 1.29  2004/09/14 14:11:10  pauly
 *ENH: Added an activation manager to main UI class, improved snake code, various UI fixes and additions
 *
 *Revision 1.28  2004/09/08 12:09:45  pauly
 *ENH: Adapting SNAP to work with stop-n-go function in finite diff. framewk
 *
 *Revision 1.27  2004/08/26 19:43:27  pauly
 *ENH: Moved the Borland code into Common folder
 *
 *Revision 1.26  2004/08/26 18:29:19  pauly
 *ENH: New user interface for configuring the UI options
 *
 *Revision 1.25  2004/08/03 23:26:32  ibanez
 *ENH: Modification for building in multple platforms. By Julien Jomier.
 *
 *Revision 1.24  2004/07/29 14:00:36  pauly
 *ENH: A new interface for changing the appearance of SNAP
 *
 *Revision 1.23  2004/07/24 19:00:06  pauly
 *ENH: Thumbnail UI for slice zooming
 *
 *Revision 1.22  2004/07/22 19:22:49  pauly
 *ENH: Large image support for SNAP. This includes being able to use more screen real estate to display a slice, a fix to the bug with manual segmentation of images larger than the window size, and a thumbnail used when zooming into the image.
 *
 *Revision 1.21  2004/07/21 18:17:45  pauly
 *ENH: Enhancements to the way that the slices are displayed
 *
 *Revision 1.20  2004/03/19 00:54:48  pauly
 *ENH: Added the ability to externally load the advection image
 *
 *Revision 1.19  2004/01/24 18:21:00  king
 *ERR: Merged warning fixes from ITK 1.6 branch.
 *
 *Revision 1.18.2.1  2004/01/24 18:16:50  king
 *ERR: Fixed warnings.
 *
 *Revision 1.18  2003/12/12 19:34:01  pauly
 *FIX: Trying to get everything to compile again after API changes
 *
 *Revision 1.17  2003/12/10 23:20:15  hjohnson
 *UPD: Code changes to allow compilation under linux.
 *
 *Revision 1.16  2003/12/10 02:21:14  lorensen
 *ENH: Spacing is now a FixedArray.
 *
 *Revision 1.15  2003/12/07 21:19:32  pauly
 *ENH: SNAP can now resample the segmentation ROI, facilitating
 *multires segmentation and segmentation of anisotropic images
 *
 *Revision 1.14  2003/12/07 19:48:41  pauly
 *ENH: Resampling, multiresolution
 *
 *Revision 1.13  2003/11/29 17:06:48  pauly
 *ENH: Minor Help issues
 *
 *Revision 1.12  2003/11/29 14:02:42  pauly
 *FIX: History list and file associations faili with spaces in filenames
 *
 *Revision 1.11  2003/11/10 00:27:26  pauly
 *FIX: Bug with linear interpolation in PDE solver
 *ENH: Help viewer and tutorial
 *
 *Revision 1.10  2003/10/09 22:45:14  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.9  2003/10/06 12:30:00  pauly
 *ENH: Added history lists, remembering of settings, new snake parameter preview
 *
 *Revision 1.8  2003/10/02 20:57:46  pauly
 *FIX: Made sure that the previous check-in compiles on Linux
 *
 *Revision 1.7  2003/10/02 18:43:47  pauly
 *FIX: Fixed crashes with using vtkContourFilter
 *
 *Revision 1.6  2003/10/02 14:55:52  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.5  2003/09/16 16:10:17  pauly
 *FIX: No more Internal compiler errors on VC
 *FIX: Intensity curve no longer crashes
 *ENH: Histogram display on intensity curve window
 *
 *Revision 1.4  2003/09/15 19:06:58  pauly
 *FIX: Trying to get last changes to compile
 *
 *Revision 1.3  2003/09/15 17:32:19  pauly
 *ENH: Removed ImageWrapperImplementation classes
 *
 *Revision 1.2  2003/09/13 15:18:01  pauly
 *FIX: Got SNAP to work properly with different image orientations
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.5  2003/08/28 22:58:30  pauly
 *FIX: Erratic scrollbar behavior
 *
 *Revision 1.4  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.3  2003/08/27 14:03:22  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:46  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:50  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.1  2003/07/11 23:33:57  pauly
 **** empty log message ***
 *
 *Revision 1.20  2003/07/10 14:30:26  pauly
 *Integrated ITK into SNAP level set segmentation
 *
 *Revision 1.19  2003/07/01 16:53:59  pauly
 **** empty log message ***
 *
 *Revision 1.18  2003/06/23 23:59:32  pauly
 *Command line argument parsing
 *
 *Revision 1.17  2003/06/14 22:42:06  pauly
 *Several changes.  Started working on implementing the level set function
 *in ITK.
 *
 *Revision 1.16  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.15  2003/06/08 16:11:42  pauly
 *User interface changes
 *Automatic mesh updating in SNAP mode
 *
 *Revision 1.14  2003/05/22 17:36:19  pauly
 *Edge preprocessing settings
 *
 *Revision 1.13  2003/05/17 21:39:30  pauly
 *Auto-update for in/out preprocessing
 *
 *Revision 1.12  2003/05/14 18:33:58  pauly
 *SNAP Component is working. Double thresholds have been enabled.  Many other changes.
 *
 *Revision 1.11  2003/05/12 02:51:08  pauly
 *Got code to compile on UNIX
 *
 *Revision 1.10  2003/05/08 21:59:05  pauly
 *SNAP is almost working
 *
 *Revision 1.9  2003/05/07 19:14:46  pauly
 *More progress on getting old segmentation working in the new SNAP.  Almost there, region of interest and bubbles are working.
 *
 *Revision 1.8  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.7  2003/04/29 14:01:42  pauly
 *Charlotte Trip
 *
 *Revision 1.6  2003/04/25 02:58:29  pauly
 *New window2d model with InteractionModes
 *
 *Revision 1.5  2003/04/23 20:36:23  pauly
 **** empty log message ***
 *
 *Revision 1.4  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/18 00:25:37  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/04/16 05:04:17  pauly
 *Incorporated intensity modification into the snap pipeline
 *New IRISApplication
 *Random goodies
 *
 *Revision 1.2  2003/04/01 18:20:56  pauly
 **** empty log message ***
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.63  2002/05/08 17:32:57  moon
 *I made some changes Guido wanted in the GUI, including removing
 *Turello/Sapiro/Schlegel options (I only hid them, the code is all still there), changing a bunch of the ranges, etc. of the snake parameters.
 *
 *Revision 1.62  2002/04/28 17:29:43  scheuerm
 *Added some documentation
 *
 *Revision 1.61  2002/04/27 18:30:03  moon
 *Finished commenting
 *
 *Revision 1.60  2002/04/27 17:48:33  bobkov
 *Added comments
 *
 *Revision 1.59  2002/04/27 00:08:27  talbert
 *Final commenting run through . . . no functional changes.
 *
 *Revision 1.58  2002/04/26 17:37:12  moon
 *Fixed callback on save preproc dialog cancel button.
 *Fixed bubble browser output.  Position was zero-based, which didn't match the 2D
 *window slice numbers (1 based), so I changed the bubble positions to be cursor
 *position +1.
 *Disallowed starting snake window if current label in not visible.
 *Put in Apply+ button in threshold dialog, which changes seg overlay to be an
 *overlay of the positive voxels in the preproc data (a zero-level visualization).
 *Added more m_OutMessage and m_OutMessage messages.
 *
 *Revision 1.57  2002/04/25 14:13:13  moon
 *Enabled render options in snake window.
 *Changed snake params dialog slider (messed one up last time)
 *Hide r_ params in snake params dialog with in/out snake (they don't apply)
 *Calling segment3D with clear color is not allowed (error dialog)
 *
 *Revision 1.56  2002/04/24 19:50:22  moon
 *Pulled LoadGreyFileCallback out of GUI into UserInterfaceLogic, made modifications due
 *to change in ROI semantics.  Before, the ROI was from ul to lr-1, which is a bad
 *decision.  I changed everything to work with a ROI that is inclusive, meaning
 *that all voxels from ul through lr inclusive are part of the ROI. This involved
 *a lot of small changes to a lot of files.
 *
 *Revision 1.55  2002/04/24 17:10:33  bobkov
 *Added some changes to OnSnakeStartAction(),
 *OnAcceptInitializationAction() and
 *OnRestartInitializationAction()
 *so that ClearScreen() method is called on
 *m_IRISWindow3D and m_SNAPWindow3D to clear the glLists and
 *the previous segmentation is not shown in the 3D window
 *
 *Revision 1.54  2002/04/24 14:54:32  moon
 *Changed the ranges of some of the snake parameters after talking with Guido.
 *Put in a line to update mesh when the update continuously checkbox is checked.
 *
 *Revision 1.53  2002/04/24 14:13:26  moon
 *Implemented separate brightness/contrast settings for grey/preproc data
 *
 *Revision 1.52  2002/04/24 01:05:00  talbert
 *Changed IRIS2000 labels to SnAP.
 *
 *Revision 1.51  2002/04/23 21:56:50  moon
 *small bug fix with the snake params and the global state.  wasn't getting
 *the sapiro/turello/etc. option from the dialog to put into the global state.
 *
 *Revision 1.50  2002/04/23 03:19:40  talbert
 *Made some changes so that the load preproc menu option is unuseable once
 *the snake starts.
 *
 *Revision 1.49  2002/04/22 21:54:39  moon
 *Closed dialogs when accept/restart initialization is pressed, or snake type is
 *switched.
 *
 *Revision 1.48  2002/04/22 21:24:20  talbert
 *Small changes so that error messages for preprocessing loading appeared in
 *the correct status bar.
 *
 *Revision 1.47  2002/04/20 21:56:47  talbert
 *Made it impossible to save preprocessed data when it doesn't make sense
 *(if no preprocessing has been done since the last preproc load or since
 *the snake win opened).  Moved some checks for data type validity out of
 *the callbacks and into the Vox and SnakeVoxData classes where they belong.
 *
 *Revision 1.46  2002/04/19 23:03:59  moon
 *Changed more stuff to get the snake params state synched with the global state.
 *Changed the range of ground in snake params dialog.
 *Removed the use_del_g stuff, since it's really not necessary, I found out.
 *
 *Revision 1.45  2002/04/19 20:34:58  moon
 *Made preproc dialogs check global state and only preproc if parameters have changed.
 *So no if you hit apply, then ok, it doesn't re process on the ok.
 *
 *Revision 1.44  2002/04/18 21:36:51  scheuerm
 *Added documentation for my recent changes.
 *Fixed inverted display of edge preprocessing.
 *
 *Revision 1.43  2002/04/18 21:14:03  moon
 *I had changed the Cancel buttons to be Close on the Filter dialogs, and I changed
 *the names of the callbacks in GUI, but not in UserInterfaceLogic.  So I just hooked them
 *up so the dialogs get closed.
 *
 *Revision 1.42  2002/04/18 21:04:51  moon
 *Changed the IRIS window ROI stuff.  Now the ROI is always valid if an image is
 *loaded, but there is a toggle to show it or not.  This will work better with
 *Konstantin's addition of being able to drag the roi box.
 *
 *I also changed a bunch of areas where I was calling InitializeSlice for the 2D windows,
 *when this is not at all what I should have done.  Now those spots call
 *MakeSegTextureCurrent, or MakeGreyTextureCurrent.  This means that the view is not
 *reset every time the snake steps, the preproc/orig radio buttons are changed, etc.
 *
 *Revision 1.41  2002/04/16 18:54:32  moon
 *minor bug with not stopping snake when play is pushed, and then other
 *buttons are pushed.  Also added a function that can be called when the user
 *clicks the "X" on a window, but it's not what we want, I don't think.  The
 *problem is if the user clicks the "X" on the snake window when a "non modal"
 *dialog is up, all the windows close, but the program doesn't quit.  I think
 *it's a bug in FLTK, but I can't figure out how to solve it.
 *
 *Revision 1.40  2002/04/16 14:44:49  moon
 *Changed bubbles to be in world coordinates instead of image coordinates.
 *
 *Revision 1.39  2002/04/16 13:07:56  moon
 *Added tooltips to some widgets, made minor changes to enabling/disabling of
 *widgets, clearing 3D window when initialization is restarted in snake window,
 *changed kappa in edge preproc dialog to be [0..1] range instead of [0..3]
 *
 *Revision 1.38  2002/04/14 22:02:54  scheuerm
 *Changed loading dialog for preprocessed image data. Code restructuring
 *along the way: Most important is addition of
 *SnakeVoxDataClass::ReadRawPreprocData()
 *
 *Revision 1.37  2002/04/13 17:43:48  moon
 *Added some initslice calls to Win2Ds, so the redraw problem comming back
 *from snake to iris window (where the whole 2D window is yellow) would go away.
 *
 *Revision 1.36  2002/04/13 16:20:08  moon
 *Just put in a bunch of debug printouts.  They'll have to come out eventually.
 *
 *Revision 1.35  2002/04/10 21:20:16  moon
 *just added debug comments.
 *
 *Revision 1.34  2002/04/10 20:19:40  moon
 *got play and stop vcr buttons to work.
 *put in lots of comments.
 *
 *Revision 1.33  2002/04/10 14:45:03  scheuerm
 *Added documentation to the methods I worked on.
 *
 *Revision 1.32  2002/04/09 21:56:42  bobkov
 *
 *modified Step button callback to display snake in 3d window
 *
 *Revision 1.31  2002/04/09 19:32:22  talbert
 *Added comments to the save and load preprocessed functions.  Checked that
 *only float files entered as preprocessed.  Made some small cosmetic
 *changes:  loading a file switches to preproc view and sets snake mode.
 *
 *Revision 1.30  2002/04/09 18:59:33  moon
 *Put in dialog to change snake parameters.  Also implemented Rewind button, which
 *now restarts the snake.  It seems for now that changing snake parameters restarts
 *the snake.  I don't know if this is the way it has to be, or I just did something
 *wrong in snakewrapper.  I'll have to check with Sean.
 *
 *Revision 1.29  2002/04/09 17:59:37  talbert
 *Made changes to LoadPreprocessedCallback which allowed edge detection
 *preproc data to be loaded correctly.
 *
 *Revision 1.28  2002/04/09 03:48:51  talbert
 *Changed some functionality in the LoadPreprocessedCallback() so that it
 *would work with floating point data being loaded.  Most of the stuff
 *is uncommented, hackish, and limited in its testing beyond verification
 *that it displays on the screen with the right values.  These changes
 *will have to be cleaned up.
 *
 *Revision 1.27  2002/04/08 13:32:35  talbert
 *Added a preprocessed save dialog box as well as a save preprocessed menu
 *option in the snake window.  Added the code necessary to implement the
 *GUI side of saving.
 *
 *Revision 1.26  2002/04/07 02:22:49  scheuerm
 *Improved handling of OK and Apply buttons in preprocessing dialogs.
 *
 *Revision 1.23  2002/04/05 03:42:29  scheuerm
 *Thresholding sort of works. Steepness needs to be made configurable.
 *
 *Revision 1.21  2002/04/04 15:30:08  moon
 *Put in code to get StepSize choice box filled with values and working.
 *AcceptSegment button callback puts snake seg data into full_data (IRIS)
 *Fixed a couple more UI cosmetic things.
 *
 *Revision 1.20  2002/04/03 22:12:07  moon
 *Added color chip, image probe, seg probe to snake window, although seg probe
 *maybe shouldn't be there.  added update continuously checkbox to 3Dwindow.
 *changes accept/restart to be on top of each other, and one is shown at a time,
 *which I think is more intuitive.
 *changed snake iteration field to be text output.  added callback for step size
 *choice.
 *
 *Revision 1.19  2002/04/02 23:51:17  scheuerm
 *Gradient magnitude preprocessing is implemented. Stupid, stupid VTK.
 *Adjusted the range and resolution of the sigma slider. Apply button
 *still doesn't do anything but I think we don't need it.
 *
 *Revision 1.18  2002/04/02 15:12:43  moon
 *Put code in the step vcr button.  Now the snake can be "stepped"
 *
 *Revision 1.17  2002/04/01 22:29:54  moon
 *Modified OnAcceptInitializationAction, added functionality to
 *OnRestartInitializationAction
 *
 *Revision 1.16  2002/03/29 20:17:25  scheuerm
 *Implemented remapping of preprocessed data to (-1,1). The mapping can
 *be changed by altering the parameters to RemapPreprocData(...) in
 *LoadPreprocessedDataCallback() (UserInterfaceLogic.cpp).
 *
 *Revision 1.15  2002/03/29 03:33:29  scheuerm
 *Loaded preprocessed data is now converted to float. No remapping yet.
 *Stupid VTK. Added vtkImageDeepCopyFloat which copies the region of
 *interest out of a gray image and converts it to float as it goes.
 *
 *Revision 1.14  2002/03/27 17:59:40  moon
 *changed a couple things.  nothing big. a callback in .fl was bool return type
 *which didn't compile in windows. this is the version I think will work for a
 *demo for Kye
 *
 *Revision 1.13  2002/03/27 17:05:04  talbert
 *Made changes necessary to compile in Windows 2000 using Microsoft Visual C++ 6.0.
 *GUI.cpp - needed to return something from function LoadPreprocessedCallback()
 *UserInterfaceLogic.cpp - moved definitions of for loop control variables outside of loop for
 *scope reasons.
 *SnakeWrapper.cpp - changed outdt1->data to *outdt1 and outdt2->data to *outdt because
 *these variables are float, not structures.  Also changed a line using snprintf to
 *sprintf because snprintf is a GNU extension.
 *Added the files snake32.dsp and snake32.dsw for compiling in Windows 2000.
 *
 *Revision 1.12  2002/03/27 15:04:26  moon
 *Changed a bunch of stuff so that the state was basically reset when the snake
 *window is hidden (accept or cancel segmentation), and then opened again.  for
 *example, the bubbles browser needed to be emptied, the active/inactive groups
 *needed to be set to the defaults again, the radio button for the preproc
 *data needed to be turned off (so original data is shown), etc.
 *
 *Added code to the acceptinitialization button that converts bubble information
 *into binary snake initialization image, and previous segmentation info of the
 *same label should also be preserved (i.e. segmentation info that comes from
 *IRIS can be used for the initialization as well as bubbles). The snake is
 *initialized, and the controls are activated.
 *
 *Still need to code the resetinitialization so that the bubble stuff, etc. is re-
 *enabled.
 *
 *None of the vcr buttons do anything, still.
 *
 *Revision 1.11  2002/03/26 19:22:14  moon
 *I don't think I really changed anything, but I had updated, and it tried to "merge" versions with and without ^M endline characters
 *
 *Revision 1.10  2002/03/26 18:16:32  scheuerm
 *Added loading and display of preprocessed data:
 *- added vtkImageDeepCopy function
 *- added flags indicating which dataset to display in GlobalState
 *- added flag indicating whether to load gray or preprocessed data
 *  in the GUI class
 *
 *Revision 1.9  2002/03/25 02:15:57  scheuerm
 *Added loading of preprocessed data. It isn't being converted
 *to floats yet. It's not possible to actually display the data
 *right now.
 *
 *Revision 1.8  2002/03/24 19:27:46  talbert
 *Added callback the preprocess button to show dialog boxes for filtering.  Added callbacks for buttons in filtering dialog boxes.  Modified the AddBubbles callback so that the newest bubble is selected in the Bubble Browser.  m_OutAboutCompiled and ran to verify that new bubbles are selected and that the dialogs appear over the
 *3d window.  talbert s f
 *
 *Revision 1.7  2002/03/22 16:44:16  moon
 *added OpenGL display of bubbles in Window2D_s::draw
 *
 *Revision 1.6  2002/03/21 15:45:46  bobkov
 *implemented callbacks for buttons AddBubble and RemoveBubble, implemented callbacks for Radius slider and ActiveBubble browser, created methods getBubbles and getNumberOfBubbles   e
 *
 *Revision 1.5  2002/03/19 19:35:32  moon
 *added snakewrapper to makefile so it gets compiled. started putting in callback,
 *etc. for snake vcr buttons.  added snake object to IrisGlobals, instantiated in Main
 *
 *Revision 1.4  2002/03/19 17:47:10  moon
 *added some code to disable widgets, make the radio buttons work, etc. in the snake window.  fixed the quit callback from the snake window to work (crashed before)
 *changed the [accept/restart]bubble_button widgets to be acceptinitialization_button and added callbacks (empty).
 *
 *Revision 1.3  2002/03/08 14:06:29  moon
 *Added Header and Log tags to all files
 **/

