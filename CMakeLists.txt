#############################################
# PROJECT: SNAP                             #
#############################################
PROJECT(SNAP)

SUBDIRS(ProgramData)


#########################################################
# FIND PACKAGES IF BUILDING OUTSIDE INSIGHTAPPLICATIONS #
#########################################################
IF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )
  INCLUDE(${SNAP_SOURCE_DIR}/CMake/standalone.cmake)
ENDIF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )

#############################################
# SOURCE FILE SPECIFICATION                 #
#############################################

# The part of the source code devoted to the SNAP application logic
# is organized into a separate library
SET(LOGIC_CXX
  Common/CommandLineArgumentParser.cxx
  Common/IRISException.cxx
  Common/Registry.cxx
  Common/SNAPCommon.cxx
  Common/SystemInterface.cxx
  Common/ITKExtras/itkVoxBoCUBImageIO.cxx
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.cxx
  Logic/Common/ImageCoordinateGeometry.cxx
  Logic/Common/ImageCoordinateTransform.cxx
  Logic/Common/SNAPRegistryIO.cxx
  Logic/Common/ColorLabelTable.cxx
  Logic/Common/SNAPSegmentationROISettings.cxx
  Logic/Framework/GlobalState.cxx
  Logic/Framework/IRISApplication.cxx
  Logic/Framework/IRISImageData.cxx
  Logic/Framework/SNAPImageData.cxx
  Logic/ImageWrapper/GreyImageWrapper.cxx
  Logic/ImageWrapper/GuidedImageIO.cxx
  Logic/ImageWrapper/LabelImageWrapper.cxx
  Logic/ImageWrapper/LevelSetImageWrapper.cxx
  Logic/ImageWrapper/SpeedColorMap.cxx
  Logic/ImageWrapper/SpeedImageWrapper.cxx
  Logic/LevelSet/SnakeParameters.cxx
  Logic/Mesh/AllPurposeProgressAccumulator.cxx
  Logic/Mesh/IRISMeshPipeline.cxx
  Logic/Mesh/LevelSetMeshPipeline.cxx
  Logic/Mesh/MeshObject.cxx
  Logic/Mesh/MeshOptions.cxx
  Logic/Mesh/VTKMeshPipeline.cxx
  Logic/Preprocessing/EdgePreprocessingSettings.cxx
  Logic/Preprocessing/ThresholdSettings.cxx
  Logic/Slicing/IntensityCurveVTK.cxx
)

# The headers for the Logic code
SET(LOGIC_HEADERS
  Common/CommandLineArgumentParser.h
  Common/Credits.h
  Common/IRISException.h
  Common/IRISVectorTypes.h
  Common/IRISVectorTypesToITKConversion.h
  Common/IRISVectorTypes.txx
  Common/Registry.h
  Common/SNAPCommon.h
  Common/SNAPOpenGL.h
  Common/SystemInterface.h
  Common/ITKExtras/itkVoxBoCUBImageIO.h
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.h
  Logic/Common/ColorLabel.h
  Logic/Common/ColorLabelTable.h
  Logic/Common/ImageCoordinateGeometry.h
  Logic/Common/ImageCoordinateTransform.h
  Logic/Common/ImageRayIntersectionFinder.h
  Logic/Common/ImageRayIntersectionFinder.txx
  Logic/Common/SNAPRegistryIO.h
  Logic/Common/SNAPSegmentationROISettings.h
  Logic/Framework/GlobalState.h
  Logic/Framework/IRISApplication.h
  Logic/Framework/IRISImageData.h
  Logic/Framework/SNAPImageData.h
  Logic/ImageWrapper/GreyImageWrapper.h
  Logic/ImageWrapper/GuidedImageIO.h
  Logic/ImageWrapper/ImageIORoutines.h
  Logic/ImageWrapper/ImageWrapper.h
  Logic/ImageWrapper/ImageWrapper.txx
  Logic/ImageWrapper/LabelImageWrapper.h
  Logic/ImageWrapper/LevelSetImageWrapper.h
  Logic/ImageWrapper/SpeedColorMap.h
  Logic/ImageWrapper/SpeedImageWrapper.h
  Logic/LevelSet/LevelSetExtensionFilter.h
  Logic/LevelSet/SnakeParameters.h
  Logic/LevelSet/SignedDistanceFilter.h
  Logic/LevelSet/SignedDistanceFilter.txx
  Logic/LevelSet/SNAPAdvectionFieldImageFilter.h
  Logic/LevelSet/SNAPAdvectionFieldImageFilter.txx
  Logic/LevelSet/SNAPLevelSetDriver.h
  Logic/LevelSet/SNAPLevelSetDriver.txx
  Logic/LevelSet/SNAPLevelSetFunction.h
  Logic/LevelSet/SNAPLevelSetFunction.txx
  Logic/LevelSet/SNAPLevelSetStopAndGoFilter.h
  Logic/LevelSet/SNAPLevelSetStopAndGoFilter.txx
  Logic/Mesh/IRISMeshPipeline.h
  Logic/Mesh/LevelSetMeshPipeline.h
  Logic/Mesh/MeshObject.h
  Logic/Mesh/MeshOptions.h
  Logic/Mesh/VTKMeshPipeline.h
  Logic/Preprocessing/EdgePreprocessingImageFilter.h
  Logic/Preprocessing/EdgePreprocessingImageFilter.txx
  Logic/Preprocessing/EdgePreprocessingSettings.h
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.h
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.txx
  Logic/Preprocessing/ThresholdSettings.h
  Logic/Slicing/IntensityCurveInterface.h
  Logic/Slicing/IntensityCurveVTK.h
  Logic/Slicing/IRISSlicer.h
  Logic/Slicing/IRISSlicer.txx
  Logic/Slicing/UnaryFunctorCache.h
  Logic/Slicing/UnaryFunctorCache.txx
)

# These files contain the user interface source code
SET(UI_CXX
  UserInterface/BasicComponents/ColorMapBox.cxx
  UserInterface/BasicComponents/FLTKCanvas.cxx
  UserInterface/BasicComponents/FunctionPlot2DBox.cxx
  UserInterface/BasicComponents/FunctionPlot2D.cxx
  UserInterface/BasicComponents/InteractionModeClient.cxx
  UserInterface/BasicComponents/RecursiveInteractionMode.cxx
  UserInterface/BasicComponents/SnakeParametersPreviewBox.cxx
  UserInterface/BasicComponents/SnakeParametersPreviewPipeline.cxx
  UserInterface/Common/SNAPAppearanceSettings.cxx
  UserInterface/ImageIOWizard/GreyImageIOWizardLogic.cxx
  UserInterface/ImageIOWizard/PreprocessingImageIOWizardLogic.cxx
  UserInterface/ImageIOWizard/SegmentationImageIOWizardLogic.cxx
  UserInterface/MainComponents/AppearanceDialogUILogic.cxx
  UserInterface/MainComponents/HelpViewerLogic.cxx
  UserInterface/MainComponents/IntensityCurveBox.cxx
  UserInterface/MainComponents/IntensityCurveUILogic.cxx
  UserInterface/MainComponents/LabelEditorUILogic.cxx
  UserInterface/MainComponents/PreprocessingUILogic.cxx
  UserInterface/MainComponents/ResizeRegionDialogLogic.cxx
  UserInterface/MainComponents/RestoreSettingsDialogLogic.cxx
  UserInterface/MainComponents/SimpleFileDialogLogic.cxx
  UserInterface/MainComponents/SnakeParametersUILogic.cxx
  UserInterface/MainComponents/UserInterfaceLogic.cxx
  UserInterface/SliceWindow/BubblesInteractionMode.cxx
  UserInterface/SliceWindow/CrosshairsInteractionMode.cxx
  UserInterface/SliceWindow/GenericSliceWindow.cxx
  UserInterface/SliceWindow/IRISSliceWindow.cxx
  UserInterface/SliceWindow/GLToPNG.cxx
  UserInterface/SliceWindow/PaintbrushInteractionMode.cxx
  UserInterface/SliceWindow/PolygonDrawing.cxx
  UserInterface/SliceWindow/PolygonInteractionMode.cxx
  UserInterface/SliceWindow/PolygonScanConvert.cxx
  UserInterface/SliceWindow/RegionInteractionMode.cxx
  UserInterface/SliceWindow/SNAPSliceWindow.cxx
  UserInterface/SliceWindow/SliceWindowCoordinator.cxx
  UserInterface/SliceWindow/ThumbnailInteractionMode.cxx
  UserInterface/SliceWindow/ZoomPanInteractionMode.cxx
  UserInterface/Window3D/Trackball.cxx
  UserInterface/Window3D/Window3D.cxx
)

# The header files for the UI project
SET(UI_HEADERS
  UserInterface/BasicComponents/ColorMapBox.h
  UserInterface/BasicComponents/FLTKCanvas.h
  UserInterface/BasicComponents/FLTKEvent.h
  UserInterface/BasicComponents/FunctionPlot2DBox.h
  UserInterface/BasicComponents/FunctionPlot2D.h
  UserInterface/BasicComponents/InteractionMode.h
  UserInterface/BasicComponents/SnakeParametersPreviewBox.h
  UserInterface/BasicComponents/SnakeParametersPreviewPipeline.h
  UserInterface/Common/SNAPCommonUI.h
  UserInterface/Common/SNAPAppearanceSettings.h
  UserInterface/ImageIOWizard/GreyImageIOWizardLogic.h
  UserInterface/ImageIOWizard/ImageIOWizardBase.h
  UserInterface/ImageIOWizard/ImageIOWizardLogic.h
  UserInterface/ImageIOWizard/ImageIOWizardLogic.txx
  UserInterface/ImageIOWizard/PreprocessingImageIOWizardLogic.h
  UserInterface/ImageIOWizard/RestrictedImageIOWizardLogic.h
  UserInterface/ImageIOWizard/RestrictedImageIOWizardLogic.txx
  UserInterface/ImageIOWizard/SegmentationImageIOWizardLogic.h
  UserInterface/MainComponents/AppearanceDialogUIBase.h
  UserInterface/MainComponents/AppearanceDialogUILogic.h
  UserInterface/MainComponents/HelpViewerBase.h
  UserInterface/MainComponents/HelpViewerLogic.h
  UserInterface/MainComponents/IntensityCurveBox.h
  UserInterface/MainComponents/IntensityCurveUIBase.h
  UserInterface/MainComponents/IntensityCurveUILogic.h
  UserInterface/MainComponents/LabelEditorUIBase.h
  UserInterface/MainComponents/LabelEditorUILogic.h
  UserInterface/MainComponents/PreprocessingUIBase.h
  UserInterface/MainComponents/PreprocessingUILogic.h
  UserInterface/MainComponents/ResizeRegionDialogBase.h
  UserInterface/MainComponents/ResizeRegionDialogLogic.h
  UserInterface/MainComponents/RestoreSettingsDialogBase.h
  UserInterface/MainComponents/RestoreSettingsDialogLogic.h
  UserInterface/MainComponents/SnakeParametersUIBase.h
  UserInterface/MainComponents/SnakeParametersUILogic.h
  UserInterface/MainComponents/SimpleFileDialogBase.h
  UserInterface/MainComponents/SimpleFileDialogLogic.h
  UserInterface/MainComponents/UserInterfaceBase.h
  UserInterface/MainComponents/UserInterfaceLogic.h
  UserInterface/SliceWindow/BubblesInteractionMode.h
  UserInterface/SliceWindow/CrosshairsInteractionMode.h
  UserInterface/SliceWindow/GenericSliceWindow.h
  UserInterface/SliceWindow/IRISSliceWindow.h
  UserInterface/SliceWindow/GLToPNG.h
  UserInterface/SliceWindow/OpenGLSliceTexture.h
  UserInterface/SliceWindow/OpenGLSliceTexture.txx
  UserInterface/SliceWindow/PaintbrushInteractionMode.h
  UserInterface/SliceWindow/PolygonDrawing.h
  UserInterface/SliceWindow/PolygonInteractionMode.h
  UserInterface/SliceWindow/PolygonScanConvert.h
  UserInterface/SliceWindow/RegionInteractionMode.h
  UserInterface/SliceWindow/SNAPSliceWindow.h
  UserInterface/SliceWindow/SliceWindowCoordinator.h
  UserInterface/SliceWindow/ThumbnailInteractionMode.h
  UserInterface/SliceWindow/ZoomPanInteractionMode.h
  UserInterface/Window3D/Trackball.h
  UserInterface/Window3D/Window3D.h
)

# The source code for SNAP testing project
SET(TESTING_CXX
  Testing/SNAPTestDriver.cxx
  Testing/TestCompareLevelSets.cxx
  Testing/TestMain.cxx
)

# The source code for the tutorial test
SET(TESTING_TUTORIAL_CXX
  Testing/TutorialTest.cxx
)

# The headers for the testing code
SET(TESTING_HEADERS
  Testing/SNAPTestDriver.h
  Testing/TestBase.h
  Testing/TestCompareLevelSets.h
  Testing/TestImageWrapper.h
)

# The FL files for SNAP
SET(APPLICATION_FLUIDS
  UserInterface/ImageIOWizard/ImageIOWizard.fl
  UserInterface/MainComponents/AppearanceDialogUI.fl
  UserInterface/MainComponents/HelpViewer.fl
  UserInterface/MainComponents/IntensityCurveUI.fl
  UserInterface/MainComponents/LabelEditorUI.fl
  UserInterface/MainComponents/PreprocessingUI.fl
  UserInterface/MainComponents/ResizeRegionDialog.fl
  UserInterface/MainComponents/RestoreSettingsDialog.fl
  UserInterface/MainComponents/SimpleFileDialog.fl
  UserInterface/MainComponents/SnakeParametersUI.fl
  UserInterface/MainComponents/UserInterface.fl
)

# All program files
SET(PROGRAM_DATA_FILES
  ProgramData/SNAPProgramDataDirectory.txt
  ProgramData/HTMLHelp/Credits.html
  ProgramData/HTMLHelp/Tutorial.html
  ProgramData/HTMLHelp/TutorialSectionEdgeBasedSegmentation.html
  ProgramData/HTMLHelp/TutorialSectionIntroduction.html
  ProgramData/HTMLHelp/TutorialSectionIntroductionToAutomatic.html
  ProgramData/HTMLHelp/TutorialSectionLoadingImages.html
  ProgramData/HTMLHelp/TutorialSectionManualSegmentation.html
  ProgramData/HTMLHelp/TutorialSectionRegionSegmentation.html
  ProgramData/HTMLHelp/TutorialSectionTipsAndTricks.html
  ProgramData/HTMLHelp/TutorialSectionViewingImages.html
  ProgramData/HTMLHelp/TutorialSectionVolumes.html
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesPicture.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesPipeline01.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesPipeline02.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesPipeline03.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesPipeline04.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroEdgesScale.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionScale.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsExample.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsHisto.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsRemapFunction.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsSegment01.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsSegment02.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsSegment03.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroRegionsSegment04.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeConcept01.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeConcept02.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeConcept03.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeConcept04.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeMerge01.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeMerge02.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeMerge03.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeMerge04.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeVelocities.gif
  ProgramData/HTMLHelp/Artwork/ttAutoIntroSnakeVelocitiesMotion01.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeBubbles.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeFeatureImage.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeFilterBox.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeLeaky.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeNoLeaks.gif
  ProgramData/HTMLHelp/Artwork/ttEdgeParameters1.gif
  ProgramData/HTMLHelp/Artwork/ttEdgePipeline1.gif
  ProgramData/HTMLHelp/Artwork/ttIconAction.gif
  ProgramData/HTMLHelp/Artwork/ttIconAttention.gif
  ProgramData/HTMLHelp/Artwork/ttIconExpert.gif
  ProgramData/HTMLHelp/Artwork/ttIconTip.gif
  ProgramData/HTMLHelp/Artwork/ttIntroScreenLayout.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageBrowser.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageMenuBar.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageResult.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageWizardOrientation.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageWizardPreview.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageWizardPreviewWithFile.gif
  ProgramData/HTMLHelp/Artwork/ttLoadImageWizardSummary.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelEditor.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelEditorChanged.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelEditorLoaded.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelFileDialog.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelSubPanel.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentLabelSubPanel02.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentPaintClosePath.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentPaintFill.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentPaintPaste.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentPaintSelectSome.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentPaintStart.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentResult.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentRightCaudate.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentSaveWizard.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentSaveWizardFilled.gif
  ProgramData/HTMLHelp/Artwork/ttManualSegmentToolbarManual.gif
  ProgramData/HTMLHelp/Artwork/ttRegion3DImaginaryLine.gif
  ProgramData/HTMLHelp/Artwork/ttRegion3DScalpelLine.gif
  ProgramData/HTMLHelp/Artwork/ttRegion3DScalpelResult.gif
  ProgramData/HTMLHelp/Artwork/ttRegion3DTools.gif
  ProgramData/HTMLHelp/Artwork/ttRegionAddBubble01.gif
  ProgramData/HTMLHelp/Artwork/ttRegionAddBubble02.gif
  ProgramData/HTMLHelp/Artwork/ttRegionFunctionForVentricles.gif
  ProgramData/HTMLHelp/Artwork/ttRegionNewInterface.gif
  ProgramData/HTMLHelp/Artwork/ttRegionParameterWindow.gif
  ProgramData/HTMLHelp/Artwork/ttRegionPreprocessor01.gif
  ProgramData/HTMLHelp/Artwork/ttRegionPreprocessor02.gif
  ProgramData/HTMLHelp/Artwork/ttRegionROISelection01.gif
  ProgramData/HTMLHelp/Artwork/ttRegionROISelection02.gif
  ProgramData/HTMLHelp/Artwork/ttRegionROISelectionFinal.gif
  ProgramData/HTMLHelp/Artwork/ttRegionResult.gif
  ProgramData/HTMLHelp/Artwork/ttRegionScalpelLabels.gif
  ProgramData/HTMLHelp/Artwork/ttRegionToolbar.gif
  ProgramData/HTMLHelp/Artwork/ttRegionVentricleResult.gif
  ProgramData/HTMLHelp/Artwork/ttRegionWizard02.gif
  ProgramData/HTMLHelp/Artwork/ttRegionWizard03.gif
  ProgramData/HTMLHelp/Artwork/ttSprayPaintCross.gif
  ProgramData/HTMLHelp/Artwork/ttSprayPaintTool.gif
  ProgramData/HTMLHelp/Artwork/ttTipsAdvanced.gif
  ProgramData/HTMLHelp/Artwork/ttTipsFocusPlus.gif
  ProgramData/HTMLHelp/Artwork/ttTipsFocusResult.gif
  ProgramData/HTMLHelp/Artwork/ttTipsImageInfo.gif
  ProgramData/HTMLHelp/Artwork/ttTipsResample1.gif
  ProgramData/HTMLHelp/Artwork/ttTipsResample2.gif
  ProgramData/HTMLHelp/Artwork/ttTipsResample3.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageAppearance.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageControlPanel.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageControlPanelToolOptions01.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageControlPanelToolOptions02.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageControlPanelToolbox01.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageControlPanelToolbox02.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageDisplayOptions.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageIntensityDialog.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageIntensityDialogWindowing.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageIntensityDialogWindowingCurve.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageSliceWindowClick.gif
  ProgramData/HTMLHelp/Artwork/ttViewImageSliceWindowZoom.gif
  ProgramData/HTMLHelp/Artwork/ttVolumeStatsDialog.gif
  ProgramData/HTMLHelp/Artwork/ttVolumeStatsMesh.gif
  ProgramData/Images2D/EdgeForcesExample.png
  ProgramData/Images2D/EdgeForcesExample.hdr
  ProgramData/Images2D/EdgeForcesExample.img.gz
  ProgramData/Images2D/RegionForcesExample.png
  ProgramData/Images2D/RegionForcesExample.hdr
  ProgramData/Images2D/RegionForcesExample.img.gz
  ProgramData/Presets/SnakeParameterPreviewCurve.txt
)

#############################################
# SYSTEM-SPECIFIC SETTINGS                  #
#############################################
SET(SYSTEM_LIBS)

IF(UNIX)
  # Include the XFT libary for nice fonts
  OPTION(SNAP_USE_XFT_LIBRARY "Enable this option if you built FLTK with --enable-xft" OFF)
  MARK_AS_ADVANCED(SNAP_USE_XFT_LIBRARY)
  IF(SNAP_USE_XFT_LIBRARY)
    SET(SYSTEM_LIBS ${SYSTEM_LIBS} Xft)
  ENDIF(SNAP_USE_XFT_LIBRARY)
ENDIF(UNIX)

#############################################
# LIBRARIES AND EXTERNAL CODE               #
#############################################

# Due to a limitation in Visual studio 6.0 on the length of include directories
# that can be specified, (here we are including all the include directories from
# ITK, VTK, FLTK and SNAP), if the compiler is VS6, we copy the SNAP source files
# to a single path in the binary tree to cut down on the number of
# INCLUDE_DIRECTORIES
IF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_CXX "${SNAP_SOURCE_DIR}/*.cxx" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_H "${SNAP_SOURCE_DIR}/*.h" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_TXX "${SNAP_SOURCE_DIR}/*.txx" )
  SET( SNAP_SOURCES
            ${SNAP_GLOBBED_CXX}
            ${SNAP_GLOBBED_H}
            ${SNAP_GLOBBED_TXX}
     )
  MAKE_DIRECTORY( "${SNAP_BINARY_DIR}/src" )
  SET( CONFIGURED_SOURCE_DIRECTORY "${SNAP_BINARY_DIR}/src" )
  FOREACH( SourceFile ${SNAP_SOURCES} )
    GET_FILENAME_COMPONENT( CONFIGURED_SOURCE_FILE ${SourceFile} NAME )
    SET( CONFIGURED_SOURCE_FILE "${CONFIGURED_SOURCE_DIRECTORY}/${CONFIGURED_SOURCE_FILE}" )
    CONFIGURE_FILE( ${SourceFile} ${CONFIGURED_SOURCE_FILE}
                     COPYONLY IMMEDIATE )
  ENDFOREACH( SourceFile )
  INCLUDE_DIRECTORIES(
    ${CONFIGURED_SOURCE_DIRECTORY}
    ${ITK_DIR}/Utilities/zlib
    ${SNAP_BINARY_DIR}/UserInterface/ImageIOWizard
    ${SNAP_BINARY_DIR}/UserInterface/MainComponents
    ${FLTK_INCLUDE_PATH}
    ${OPENGL_INCLUDE_PATH}
  )

ELSE( CMAKE_GENERATOR MATCHES "Visual Studio 6" )

  # Include directories
  INCLUDE_DIRECTORIES(
    ${ITK_DIR}/Utilities/zlib
    ${SNAP_SOURCE_DIR}/Common
    ${SNAP_SOURCE_DIR}/Common/ITKExtras
    ${SNAP_SOURCE_DIR}/Logic
    ${SNAP_SOURCE_DIR}/Logic/Common
    ${SNAP_SOURCE_DIR}/Logic/Framework
    ${SNAP_SOURCE_DIR}/Logic/ImageWrapper
    ${SNAP_SOURCE_DIR}/Logic/LevelSet
    ${SNAP_SOURCE_DIR}/Logic/Mesh
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing
    ${SNAP_SOURCE_DIR}/Logic/Slicing
    ${SNAP_SOURCE_DIR}/Testing
    ${SNAP_SOURCE_DIR}/UserInterface/BasicComponents
    ${SNAP_SOURCE_DIR}/UserInterface/Common
    ${SNAP_SOURCE_DIR}/UserInterface/ImageIOWizard
    ${SNAP_SOURCE_DIR}/UserInterface/MainComponents
    ${SNAP_SOURCE_DIR}/UserInterface/MainComponents/Artwork
    ${SNAP_SOURCE_DIR}/UserInterface/SliceWindow
    ${SNAP_SOURCE_DIR}/UserInterface/Window3D
    ${SNAP_BINARY_DIR}/UserInterface/ImageIOWizard
    ${SNAP_BINARY_DIR}/UserInterface/MainComponents
    ${FLTK_INCLUDE_PATH}
    ${OPENGL_INCLUDE_PATH}
  )

ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )



# ----------------------------------------------------------------
# Define External Libraries
# ----------------------------------------------------------------

# ITK Libraries
SET(SNAP_ITK_LIBS
  ITKIO
)

# Core VTK libraries
SET(SNAP_VTK_CORE_LIBS
  vtkCommon
  vtkFiltering
  vtkGraphics
  vtkImaging
  vtkIO
)

# VTK Libraries with possible inclusion of patented code
IF(VTK_USE_PATENTED)
  SET(SNAP_VTK_LIBS vtkPatented ${SNAP_VTK_CORE_LIBS})
ELSE(VTK_USE_PATENTED)
  SET(SNAP_VTK_LIBS ${SNAP_VTK_CORE_LIBS})
ENDIF(VTK_USE_PATENTED)

# FLTK Related libraries
## Workaround for problem with FindFLTK.cmake when FLTK was built using Cmake.
IF(NOT FLTK_LIBRARIES)
  SET(FLTK_LIBRARIES  
    ${FLTK_IMAGES_LIBRARY} 
    ${FLTK_IMAGES_LIBS} 
    ${FLTK_BASE_LIBRARY} 
    ${FLTK_GL_LIBRARY} 
    ${FLTK_FORMS_LIBRARY})
ENDIF(NOT FLTK_LIBRARIES)

SET(SNAP_FLTK_LIBS ${FLTK_LIBRARIES})

# System libraries
SET(SNAP_SYSTEM_LIBS
  ${OPENGL_LIBRARIES}
  ${OPENGL_glu_LIBRARY}
  ${SYSTEM_LIBS}
)

# Designate the external libraries used by SNAP
SET(SNAP_EXTERNAL_LIBS 
  ${SNAP_ITK_LIBS} 
  ${SNAP_VTK_LIBS} 
  ${SNAP_FLTK_LIBS} 
  ${SNAP_SYSTEM_LIBS})

# *****************************************************
# SNAP Logic and UI Libraries
# *****************************************************

# Wrap the .fl files
FLTK_WRAP_UI(itksnapui ${APPLICATION_FLUIDS})

# The SNAP logic library
ADD_LIBRARY(itksnaplogic ${LOGIC_CXX})

# The user interface code library
ADD_LIBRARY(itksnapui ${UI_CXX} ${itksnapui_FLTK_UI_SRCS})

# This is experimental: it seems that shared libraries do not
# build accurately (at least on MacOS) without the following
# two lines
TARGET_LINK_LIBRARIES(itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui itksnaplogic ${SNAP_EXTERNAL_LIBS})

# Designate the SNAP internal libraries
SET(SNAP_INTERNAL_LIBS itksnapui itksnaplogic)

# *****************************************************
# Define SNAP Executables
# *****************************************************

# Disable FLTK warnings
ITK_DISABLE_FLTK_GENERATED_WARNINGS("${APPLICATION_FLUIDS}")

# Define the main SNAP executable
SET(SNAP_EXE_PARAMETERS "WIN32" UserInterface/SNAPMain.cxx)
ADD_GUI_EXECUTABLE(InsightSNAP "${SNAP_EXE_PARAMETERS}")
TARGET_LINK_LIBRARIES(InsightSNAP 
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS})

# The customized executable for creating a screen shot
OPTION(SNAP_PNG_CAPTURE_TOOL 
  "Should the screen capture tool by Hui Gary Zhang be built?" OFF)
MARK_AS_ADVANCED(SNAP_PNG_CAPTURE_TOOL)
IF(SNAP_PNG_CAPTURE_TOOL)
  SET(SNAP_CC_PNG_EXE_PARAMETERS "WIN32" UserInterface/SNAP_CC_PNG.cxx)
  ADD_GUI_EXECUTABLE(SNAP_CC_PNG "${SNAP_CC_PNG_EXE_PARAMETERS}")
  TARGET_LINK_LIBRARIES(SNAP_CC_PNG 
    ${SNAP_INTERNAL_LIBS}
    ${SNAP_EXTERNAL_LIBS})
ENDIF(SNAP_PNG_CAPTURE_TOOL)

# The testing executables
IF(BUILD_TESTING)
  
  ADD_GUI_EXECUTABLE(SNAPTests "${TESTING_CXX}")
  TARGET_LINK_LIBRARIES(SNAPTests 
    ${SNAP_INTERNAL_LIBS} 
    ${SNAP_EXTERNAL_LIBS})
    
  
  ADD_EXECUTABLE(SNAPTutorialTest ${TESTING_TUTORIAL_CXX})
  TARGET_LINK_LIBRARIES(SNAPTutorialTest 
    itksnaplogic 
    ${SNAP_EXTERNAL_LIBS})

  INSTALL_TARGETS(/bin SNAPTutorialTest)

ENDIF(BUILD_TESTING)

# ----------------------------------------------------------------
# Miscelaneous tasks (not related to link and compilation)
# ----------------------------------------------------------------

# Copy documentation from the source tree to the build tree
FOREACH(DATAFILE ${PROGRAM_DATA_FILES})
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/${DATAFILE}
    ${SNAP_BINARY_DIR}/${DATAFILE}
    COPYONLY)
ENDFOREACH(DATAFILE)

# Copy the InnoSetup installer configuration script
CONFIGURE_FILE(
  ${SNAP_SOURCE_DIR}/Utilities/InnoSetup/InstallSNAP.iss.in
  ${SNAP_BINARY_DIR}/Utilities/InnoSetup/InstallSNAP.iss)

# Add an option to install SNAP into /usr/local or something
INSTALL_TARGETS(/bin InsightSNAP)
