#############################################
# CMAKE PRELIMINARIES                       #
#############################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)  

#############################################
# PROJECT: SNAP                             #
#############################################
PROJECT(SNAP)

#############################################
# VERSION INFORMATION                       #
#############################################
# On SNAP versions.
# =================
# The SNAP version consists of four fields: major, minor, patch and qualifier
# for example, version 1.7.3-beta has major version 1, minor version 7, patch 3
# and qualifier "-beta". Major, minor and patch must be numbers, but the qualifier
# is an arbitrary string and may be blank. 
# 
# Important:
#   - The qualifier is just a descriptor. No two version should have the same 
#     major/minor/patch and different qualifiers. So it's completely wrong to 
#     release version 1.7.3-beta and then version 1.7.3. The right way to do it
#     is to have 1.7.3-beta followed by 1.7.5-rc1 followed by 1.8.0 and so on
SET(SNAP_VERSION_MAJOR 2)
SET(SNAP_VERSION_MINOR 2)
SET(SNAP_VERSION_PATCH 0)
SET(SNAP_VERSION_QUALIFIER "")
SET(SNAP_VERSION_FULL "${SNAP_VERSION_MAJOR}.${SNAP_VERSION_MINOR}.${SNAP_VERSION_PATCH}${SNAP_VERSION_QUALIFIER}")
SET(SNAP_VERSION_RELEASE_DATE "20110504")
SET(SNAP_VERSION_LAST_COMPATIBLE_RELEASE_DATE "20090731")
SET(SNAP_VERSION_RELEASE_DATE_FORMATTED "May 4, 2011")

# Shamelessly stolen from ParaView
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "itksnap-${SNAP_VERSION_FULL}-${SNAP_VERSION_RELEASE_DATE}")
IF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
  EXEC_PROGRAM(uname ARGS "-m" OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
IF(NOT DEFINED CPACK_SYSTEM_NAME)
  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
ENDIF(NOT DEFINED CPACK_SYSTEM_NAME)
IF(${CPACK_SYSTEM_NAME} MATCHES Windows)
  IF(CMAKE_CL_64)
    SET(CPACK_SYSTEM_NAME win64-${CMAKE_SYSTEM_PROCESSOR})
  ELSE(CMAKE_CL_64)
    SET(CPACK_SYSTEM_NAME win32-${CMAKE_SYSTEM_PROCESSOR})
  ENDIF(CMAKE_CL_64)
ENDIF(${CPACK_SYSTEM_NAME} MATCHES Windows)

# For Apple, we need to base the filename on the architecture
IF(CMAKE_SYSTEM_NAME MATCHES Darwin)
  STRING(REPLACE ";" "-" ARCHBIT ${CMAKE_OSX_ARCHITECTURES})
  SET(CPACK_SYSTEM_NAME "MacOS-${ARCHBIT}")
ENDIF(CMAKE_SYSTEM_NAME MATCHES Darwin)

IF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
ENDIF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)

#########################################################
# FIND PACKAGES IF BUILDING OUTSIDE INSIGHTAPPLICATIONS #
#########################################################
IF(DEFINED InsightApplications_SOURCE_DIR)
  SET(BUILD_OUTSIDE_INSIGHT_APPLICATIONS FALSE CACHE BOOL 
	"Is SNAP being built separate from InsightApplications?")
ELSE(DEFINED InsightApplications_SOURCE_DIR)
  SET(BUILD_OUTSIDE_INSIGHT_APPLICATIONS TRUE CACHE BOOL 
	"Is SNAP being built separate from InsightApplications?")
ENDIF(DEFINED InsightApplications_SOURCE_DIR)			

IF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )
  INCLUDE(${SNAP_SOURCE_DIR}/CMake/standalone.cmake)
ENDIF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )

#############################################
# SOURCE FILE SPECIFICATION                 #
#############################################

# One of the files needs to be configured (to insert version info)
CONFIGURE_FILE(
  ${SNAP_SOURCE_DIR}/Common/SNAPCommon.cxx.in
  ${SNAP_BINARY_DIR}/SNAPCommon.cxx @ONLY IMMEDIATE)

# The part of the source code devoted to the SNAP application logic
# is organized into a separate library
SET(LOGIC_CXX
  ${SNAP_BINARY_DIR}/SNAPCommon.cxx
  Common/CommandLineArgumentParser.cxx
  Common/IRISException.cxx
  Common/Registry.cxx
  Common/SystemInterface.cxx
  Common/ITKExtras/itkVoxBoCUBImageIO.cxx
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.cxx
  Logic/Common/ColorMap.cxx
  Logic/Common/ColorLabelTable.cxx
  Logic/Common/ImageCoordinateGeometry.cxx
  Logic/Common/ImageCoordinateTransform.cxx
  Logic/Common/SegmentationStatistics.cxx
  Logic/Common/SNAPRegistryIO.cxx
  Logic/Common/SNAPSegmentationROISettings.cxx
  Logic/Framework/GenericImageData.cxx
  Logic/Framework/GlobalState.cxx
  Logic/Framework/IRISApplication.cxx
  Logic/Framework/IRISImageData.cxx
  Logic/Framework/SNAPImageData.cxx
  Logic/Framework/UndoDataManager_LabelType.cxx
  Logic/ImageWrapper/GreyImageWrapper.cxx
  Logic/ImageWrapper/GuidedNativeImageIO.cxx
  Logic/ImageWrapper/LabelImageWrapper.cxx
  Logic/ImageWrapper/LevelSetImageWrapper.cxx
  Logic/ImageWrapper/RGBImageWrapper.cxx
  Logic/ImageWrapper/SpeedColorMap.cxx
  Logic/ImageWrapper/SpeedImageWrapper.cxx
  Logic/LevelSet/SnakeParameters.cxx
  Logic/Mesh/AllPurposeProgressAccumulator.cxx
  Logic/Mesh/GuidedMeshIO.cxx
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
  Common/IRISVectorTypes.txx
  Common/IRISVectorTypesToITKConversion.h
  Common/ITKExtras/itkBSplineScatteredDataPointSetToImageFilter.h
  Common/ITKExtras/itkBSplineScatteredDataPointSetToImageFilter.txx
  Common/ITKExtras/itkBinaryDiamondStructuringElement.h
  Common/ITKExtras/itkBinaryDiamondStructuringElement.txx
  Common/ITKExtras/itkCoxDeBoorBSplineKernelFunction.h
  Common/ITKExtras/itkCoxDeBoorBSplineKernelFunction.txx
  Common/ITKExtras/itkParallelSparseFieldLevelSetImageFilterBugFix.h
  Common/ITKExtras/itkParallelSparseFieldLevelSetImageFilterBugFix.txx
  Common/ITKExtras/itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.h
  Common/ITKExtras/itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.txx
  Common/ITKExtras/itkVoxBoCUBImageIO.h
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.h
  Common/Registry.h
  Common/SNAPBorlandDummyTypes.h
  Common/SNAPCommon.h
  Common/SNAPOpenGL.h
  Common/SystemInterface.h
  Logic/Common/ColorLabel.h
  Logic/Common/ColorLabelTable.h
  Logic/Common/ColorMap.h
  Logic/Common/ImageCoordinateGeometry.h
  Logic/Common/ImageCoordinateTransform.h
  Logic/Common/SegmentationStatistics.h
  Logic/Common/ImageRayIntersectionFinder.h
  Logic/Common/ImageRayIntersectionFinder.txx
  Logic/Common/SNAPRegistryIO.h
  Logic/Common/SNAPSegmentationROISettings.h
  Logic/Framework/GenericImageData.h
  Logic/Framework/GlobalState.h
  Logic/Framework/IRISApplication.h
  Logic/Framework/IRISImageData.h
  Logic/Framework/SNAPImageData.h
  Logic/Framework/UndoDataManager.h
  Logic/Framework/UndoDataManager.txx
  Logic/ImageWrapper/GreyImageWrapper.h
  Logic/ImageWrapper/ImageIORoutines.h
  Logic/ImageWrapper/ImageWrapper.h
  Logic/ImageWrapper/ImageWrapper.txx
  Logic/ImageWrapper/LabelImageWrapper.h
  Logic/ImageWrapper/LabelToRGBAFilter.h
  Logic/ImageWrapper/LevelSetImageWrapper.h
  Logic/ImageWrapper/RGBImageWrapper.h
  Logic/ImageWrapper/ScalarImageWrapper.h
  Logic/ImageWrapper/ScalarImageWrapper.txx
  Logic/ImageWrapper/SpeedColorMap.h
  Logic/ImageWrapper/SpeedImageWrapper.h
  Logic/ImageWrapper/VectorImageWrapper.h
  Logic/ImageWrapper/VectorImageWrapper.txx
  Logic/LevelSet/LevelSetExtensionFilter.h
  Logic/LevelSet/SNAPAdvectionFieldImageFilter.h
  Logic/LevelSet/SNAPAdvectionFieldImageFilter.txx
  Logic/LevelSet/SNAPLevelSetDriver.h
  Logic/LevelSet/SNAPLevelSetDriver.txx
  Logic/LevelSet/SNAPLevelSetFunction.h
  Logic/LevelSet/SNAPLevelSetFunction.txx
  Logic/LevelSet/SNAPLevelSetStopAndGoFilter.h
  Logic/LevelSet/SNAPLevelSetStopAndGoFilter.txx
  Logic/LevelSet/SignedDistanceFilter.h
  Logic/LevelSet/SignedDistanceFilter.txx
  Logic/LevelSet/SnakeParameters.h
  Logic/Mesh/AllPurposeProgressAccumulator.h
  Logic/Mesh/GuidedMeshIO.h
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
  Logic/Slicing/IRISSlicer.h
  Logic/Slicing/IRISSlicer.txx
  Logic/Slicing/IntensityCurveInterface.h
  Logic/Slicing/IntensityCurveVTK.h
  Logic/Slicing/UnaryFunctorCache.h
  Logic/Slicing/UnaryFunctorCache.txx  
)

# These files contain the user interface source code
SET(UI_CXX
  UserInterface/BasicComponents/ColorMapBox.cxx
  UserInterface/BasicComponents/ColorMapWidget.cxx
  UserInterface/BasicComponents/FLTKCanvas.cxx
  UserInterface/BasicComponents/FunctionPlot2DBox.cxx
  UserInterface/BasicComponents/FunctionPlot2D.cxx
  UserInterface/BasicComponents/IntensityCurveBox.cxx
  UserInterface/BasicComponents/InteractionModeClient.cxx
  UserInterface/BasicComponents/MetaDataTable.cxx
  UserInterface/BasicComponents/RecursiveInteractionMode.cxx
  UserInterface/BasicComponents/SnakeParametersPreviewBox.cxx
  UserInterface/BasicComponents/SnakeParametersPreviewPipeline.cxx
  UserInterface/BasicComponents/StatisticsTable.cxx
  UserInterface/Common/SNAPAppearanceSettings.cxx
  UserInterface/ImageIOWizard/ImageIOWizardLogic.cxx
  UserInterface/ImageIOWizard/RestrictedImageIOWizardLogic.cxx
  UserInterface/MainComponents/AppearanceDialogUILogic.cxx
  UserInterface/MainComponents/HelpViewerLogic.cxx
  UserInterface/MainComponents/LabelEditorUILogic.cxx
  UserInterface/MainComponents/LayerInspectorUILogic.cxx
  UserInterface/MainComponents/PreprocessingUILogic.cxx
  UserInterface/MainComponents/ReorientImageUILogic.cxx
  UserInterface/MainComponents/ResizeRegionDialogLogic.cxx
  UserInterface/MainComponents/RestoreSettingsDialogLogic.cxx
  UserInterface/MainComponents/SimpleFileDialogLogic.cxx
  UserInterface/MainComponents/SnakeParametersUILogic.cxx
  UserInterface/MainComponents/UserInterfaceLogic.cxx
  UserInterface/MeshIOWizard/MeshIOWizardUILogic.cxx
  UserInterface/SliceWindow/AnnotationInteractionMode.cxx
  UserInterface/SliceWindow/BubblesInteractionMode.cxx
  UserInterface/SliceWindow/CrosshairsInteractionMode.cxx
  UserInterface/SliceWindow/GenericSliceWindow.cxx
  UserInterface/SliceWindow/IRISSliceWindow.cxx
  UserInterface/SliceWindow/GLToPNG.cxx
  UserInterface/SliceWindow/OpenGLSliceTexture.cxx  
  UserInterface/SliceWindow/PaintbrushInteractionMode.cxx
  UserInterface/SliceWindow/PolygonDrawing.cxx
  UserInterface/SliceWindow/PolygonInteractionMode.cxx
  UserInterface/SliceWindow/PolygonScanConvert.cxx
  UserInterface/SliceWindow/PopupButtonInteractionMode.cxx
  UserInterface/SliceWindow/RegionInteractionMode.cxx
  UserInterface/SliceWindow/SNAPSliceWindow.cxx
  UserInterface/SliceWindow/SliceWindowCoordinator.cxx
  UserInterface/SliceWindow/ThumbnailInteractionMode.cxx
  UserInterface/Window3D/Trackball.cxx
  UserInterface/Window3D/Window3D.cxx
)

# The header files for the UI project
SET(UI_HEADERS
  UserInterface/BasicComponents/ColorMapBox.h
  UserInterface/BasicComponents/ColorMapWidget.h
  UserInterface/BasicComponents/FLTKCanvas.h
  UserInterface/BasicComponents/FLTKEvent.h
  UserInterface/BasicComponents/FLTKWidgetActivationManager.h
  UserInterface/BasicComponents/FunctionPlot2D.h
  UserInterface/BasicComponents/FunctionPlot2DBox.h
  UserInterface/BasicComponents/IntensityCurveBox.h
  UserInterface/BasicComponents/InteractionMode.h
  UserInterface/BasicComponents/InteractionModeClient.h
  UserInterface/BasicComponents/MetaDataTable.h  
  UserInterface/BasicComponents/RecursiveInteractionMode.h
  UserInterface/BasicComponents/SnakeParametersPreviewBox.h
  UserInterface/BasicComponents/SnakeParametersPreviewPipeline.h
  UserInterface/BasicComponents/StatisticsTable.h
  UserInterface/Common/SNAPAppearanceSettings.h
  UserInterface/Common/SNAPCommonUI.h
  UserInterface/ImageIOWizard/ImageIOWizardBase.h
  UserInterface/ImageIOWizard/ImageIOWizardLogic.h
  UserInterface/ImageIOWizard/RestrictedImageIOWizardLogic.h
  UserInterface/MainComponents/AppearanceDialogUIBase.h
  UserInterface/MainComponents/AppearanceDialogUILogic.h
  UserInterface/MainComponents/HelpViewerBase.h
  UserInterface/MainComponents/HelpViewerLogic.h
  UserInterface/MainComponents/LabelEditorUIBase.h
  UserInterface/MainComponents/LabelEditorUILogic.h
  UserInterface/MainComponents/LayerInspectorUIBase.h
  UserInterface/MainComponents/LayerInspectorUILogic.h
  UserInterface/MainComponents/PreprocessingUIBase.h
  UserInterface/MainComponents/PreprocessingUILogic.h
  UserInterface/MainComponents/ReorientImageUIBase.h
  UserInterface/MainComponents/ReorientImageUILogic.h
  UserInterface/MainComponents/ResizeRegionDialogBase.h
  UserInterface/MainComponents/ResizeRegionDialogLogic.h
  UserInterface/MainComponents/RestoreSettingsDialogBase.h
  UserInterface/MainComponents/RestoreSettingsDialogLogic.h
  UserInterface/MainComponents/SimpleFileDialogBase.h
  UserInterface/MainComponents/SimpleFileDialogLogic.h
  UserInterface/MainComponents/SnakeParametersUIBase.h
  UserInterface/MainComponents/SnakeParametersUILogic.h
  UserInterface/MainComponents/UserInterfaceBase.h
  UserInterface/MainComponents/UserInterfaceLogic.h
  UserInterface/MeshIOWizard/MeshExportSettings.h
  UserInterface/MeshIOWizard/MeshIOWizardUIBase.h
  UserInterface/MeshIOWizard/MeshIOWizardUILogic.h
  UserInterface/SliceWindow/AnnotationInteractionMode.h
  UserInterface/SliceWindow/BubblesInteractionMode.h
  UserInterface/SliceWindow/CrosshairsInteractionMode.h
  UserInterface/SliceWindow/GLToPNG.h
  UserInterface/SliceWindow/GenericSliceWindow.h
  UserInterface/SliceWindow/IRISSliceWindow.h
  UserInterface/SliceWindow/OpenGLSliceTexture.h
  UserInterface/SliceWindow/PaintbrushInteractionMode.h
  UserInterface/SliceWindow/PolygonDrawing.h
  UserInterface/SliceWindow/PolygonInteractionMode.h
  UserInterface/SliceWindow/PolygonScanConvert.h
  UserInterface/SliceWindow/PopupButtonInteractionMode.h
  UserInterface/SliceWindow/RegionInteractionMode.h
  UserInterface/SliceWindow/SNAPSliceWindow.h
  UserInterface/SliceWindow/SliceWindowCoordinator.h
  UserInterface/SliceWindow/ThumbnailInteractionMode.h
  UserInterface/Window3D/Trackball.h
  UserInterface/Window3D/Window3D.h
)

# The source code for SNAP testing project
SET(TESTING_CXX
  Testing/TestMain.cxx
  Testing/SNAPTestDriver.cxx
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
  UserInterface/MeshIOWizard/MeshIOWizardUI.fl
  UserInterface/MainComponents/AppearanceDialogUI.fl
  UserInterface/MainComponents/HelpViewer.fl
  UserInterface/MainComponents/LabelEditorUI.fl
  UserInterface/MainComponents/LayerInspectorUI.fl
  UserInterface/MainComponents/PreprocessingUI.fl
  UserInterface/MainComponents/ReorientImageUI.fl
  UserInterface/MainComponents/ResizeRegionDialog.fl
  UserInterface/MainComponents/RestoreSettingsDialog.fl
  UserInterface/MainComponents/SimpleFileDialog.fl
  UserInterface/MainComponents/SnakeParametersUI.fl
  UserInterface/MainComponents/UserInterface.fl
)

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
    ${SNAP_SOURCE_DIR}/UserInterface/MeshIOWizard
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

# Get rid of this ridiculous warning in VS8
IF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9")
  
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
  vtkRendering
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
SET(SNAP_FLTK_LIBS ${FLTK_LIBRARIES})

# System libraries
SET(SNAP_SYSTEM_LIBS
  ${OPENGL_LIBRARIES}
  ${OPENGL_glu_LIBRARY}
  ${SYSTEM_LIBS}
)

# Designate the external libraries used by SNAP
SET(SNAP_EXTERNAL_LIBS 
  ${SNAP_FLTK_LIBS} 
  ${SNAP_ITK_LIBS} 
  ${SNAP_VTK_LIBS} 
  ${SNAP_SYSTEM_LIBS})

# *****************************************************
# SNAP Logic and UI Libraries
# *****************************************************

# Wrap the .fl files
FLTK_WRAP_UI(itksnapui ${APPLICATION_FLUIDS})

# The SNAP logic library
ADD_LIBRARY(itksnaplogic ${LOGIC_CXX} ${LOGIC_HEADERS})

# The user interface code library
ADD_LIBRARY(itksnapui ${UI_CXX} ${UI_HEADERS} ${itksnapui_FLTK_UI_SRCS})

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
SET(SNAP_EXE InsightSNAP)

# Disable FLTK warnings
#ITK_DISABLE_FLTK_GENERATED_WARNINGS("${APPLICATION_FLUIDS}")

# Define the main SNAP executable
ADD_EXECUTABLE(${SNAP_EXE} WIN32 UserInterface/SNAPMain.cxx)
TARGET_LINK_LIBRARIES(${SNAP_EXE}
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS}
  )

# Define the testing EXE
ADD_EXECUTABLE(snaptest ${TESTING_CXX})
TARGET_LINK_LIBRARIES(snaptest
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS})

# ----------------------------------------------------------------
# Miscelaneous tasks (not related to link and compilation)
# ----------------------------------------------------------------
# All program files - use recursive globbing
FILE(GLOB_RECURSE PROGRAM_DATA_FILES ProgramData "*.txt" "*.html" "*.gif" "*.png" "*.img.gz" "*.hdr")

# Copy documentation from the source tree to the build tree
FOREACH(DATAFILE ${PROGRAM_DATA_FILES})
  FILE(RELATIVE_PATH SHORTNAME ${SNAP_SOURCE_DIR} ${DATAFILE})
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/${SHORTNAME}
    ${SNAP_BINARY_DIR}/${SHORTNAME}
   COPYONLY)
ENDFOREACH(DATAFILE)

#########################################################
# INSTALLATION AND PACKAGING with CPack                 #
#########################################################
INCLUDE(CMake/CustomBuildSettings.cmake)

# Generate forward shared executable
SUBDIRS(Utilities/Forwarding)

# Install the SNAP executable in the appropriate place.

# Windows (Microsoft Visual Studio)
IF(WIN32 AND NOT UNIX)
  INSTALL(TARGETS ${SNAP_EXE} RUNTIME DESTINATION bin)
  SET(SNAP_DATA_INSTALL_DIR ".")
ENDIF(WIN32 AND NOT UNIX)

# Apple
IF(APPLE)
  # Copy executable into the bundle
  SET(SNAP_MAIN_INSTALL_DIR ITK-SNAP.app/Contents/MacOS)
  SET(SNAP_DATA_INSTALL_DIR ${SNAP_MAIN_INSTALL_DIR})
  INSTALL(TARGETS ${SNAP_EXE} RUNTIME DESTINATION ${SNAP_MAIN_INSTALL_DIR})

  # Configure the XML file
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/Info.plist
    ${SNAP_BINARY_DIR}/Utilities/MacOS/BundleResources/Info.plist)
  INSTALL(FILES ${SNAP_BINARY_DIR}/Utilities/MacOS/BundleResources/Info.plist
    DESTINATION ITK-SNAP.app/Contents) 
  INSTALL(FILES ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap.icns
    DESTINATION ITK-SNAP.app/Contents/Resources)
ENDIF(APPLE)

# Other UNIX
IF(UNIX AND NOT APPLE)
  SET(SNAP_MAIN_INSTALL_DIR lib/snap-${SNAP_VERSION_FULL})
  SET(SNAP_DATA_INSTALL_DIR ${SNAP_MAIN_INSTALL_DIR})
  INSTALL(TARGETS ${SNAP_EXE} RUNTIME DESTINATION ${SNAP_MAIN_INSTALL_DIR})
ENDIF(UNIX AND NOT APPLE)

# Install the Program Data files
INSTALL(DIRECTORY ${SNAP_BINARY_DIR}/ProgramData DESTINATION ${SNAP_DATA_INSTALL_DIR})

# On Win32, we must include the redistributable
IF(MSVC80 OR MSVC90)
  FIND_PROGRAM(VCREDIST_EXE vcredist_x86.exe vcredist_x64.exe)
  IF(VCREDIST_EXE)
    GET_FILENAME_COMPONENT(VCREDIST_NAME ${VCREDIST_EXE} NAME)
    INSTALL(FILES ${VCREDIST_EXE} DESTINATION bin)
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS 
      "ExecWait '\\\"$INSTDIR\\\\bin\\\\${VCREDIST_NAME}\\\" /q:a'")
  ENDIF(VCREDIST_EXE)
ENDIF(MSVC80 OR MSVC90)

# Allow package generation
SET(CPACK_PACKAGE_NAME "itksnap")
SET(CPACK_PACKAGE_CONTACT "Paul A. Yushkevich")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ITK-SNAP 3D Image Segmentation Tool")
SET(CPACK_PACKAGE_VENDOR "itksnap.org")
SET(CPACK_PACKAGE_VERSION_MAJOR "${SNAP_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${SNAP_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${SNAP_VERSION_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "itksnap-${SNAP_VERSION_FULL}")

# Show GPL license
SET(CPACK_RESOURCE_FILE_LICENSE "${SNAP_SOURCE_DIR}/COPYING")

IF(WIN32 AND NOT UNIX)

  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_GENERATOR "NSIS")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "InsightSNAP.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ITK-SNAP")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.itksnap.org")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.itksnap.org/credits.php")
  SET(CPACK_NSIS_MODIFY_PATH OFF)
  
  # Give it a windowsy directory name
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "ITK-SNAP ${SNAP_VERSION_MAJOR}.${SNAP_VERSION_MINOR}")
  
  # On Win32, the executable is the actual exe
  SET(CPACK_PACKAGE_EXECUTABLES InsightSNAP "ITK-SNAP")
  
ELSE(WIN32 AND NOT UNIX)

  # Set the generator to either STGZ or Apple
  IF(NOT APPLE)
    SET(CPACK_GENERATOR "TGZ")
  ELSE(NOT APPLE)
    SET(CPACK_GENERATOR "ZIP")
  ENDIF(NOT APPLE)

  # Executable is the forward sharing exe
  SET(CPACK_PACKAGE_EXECUTABLES "itksnap" "ITK-SNAP")

ENDIF(WIN32 AND NOT UNIX)

# Figure out the extension of the binary
MESSAGE(STATUS "Generator: ${CPACK_GENERATOR}")
IF(CPACK_GENERATOR STREQUAL "NSIS")
  SET(CPACK_EXTENSION "exe")
ELSEIF(CPACK_GENERATOR STREQUAL "ZIP")
  SET(CPACK_EXTENSION "zip")
ELSEIF(CPACK_GENERATOR STREQUAL "TGZ")
  SET(CPACK_EXTENSION "tar.gz")
ENDIF(CPACK_GENERATOR STREQUAL "NSIS")

# The filename of the installable package
SET(CPACK_TARGET_FILENAME ${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION})
SET(CPACK_TARGET ${SNAP_BINARY_DIR}/${CPACK_TARGET_FILENAME})

# Call CPACK
INCLUDE(CPack)
INCLUDE(CTest)

#########################################################
# Automatic Uploading of Nightly Packages               #
#########################################################
FIND_PROGRAM(SCP_PROGRAM NAMES scp DOC "Location of the scp program (optional)")
MARK_AS_ADVANCED(SCP_PROGRAM)

SET(SCP_ARGUMENTS "-v" CACHE STRING "Optional arguments to the scp command for uploads to SourceForge")
MARK_AS_ADVANCED(SCP_ARGUMENTS)

SET(SCP_USERNAME "" CACHE STRING "SourceForge.net account id for uploads")
MARK_AS_ADVANCED(SCP_USERNAME)

SET(NIGHTLY_TARGET "itksnap-nightly-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")

SET(SCP_ROOT "frs.sourceforge.net:/home/frs/project/i/it/itk-snap/itk-snap")

ADD_CUSTOM_TARGET(upload_nightly 
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS} ${CPACK_TARGET_FILENAME} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Nightly/${NIGHTLY_TARGET}
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_TARGET} to SourceForge.net as ${NIGHTLY_TARGET}")

ADD_CUSTOM_TARGET(upload_experimental 
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS} ${CPACK_TARGET_FILENAME} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Experimental
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_TARGET} to SourceForge.net to Experimental directory")

