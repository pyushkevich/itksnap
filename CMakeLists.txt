#--------------------------------------------------------------------------------
# PROJECT: SNAP                             
#--------------------------------------------------------------------------------
# This CMake file is modeled after QtTest example project from
# http://www.cmake.org/Wiki/BundleUtilitiesExample

PROJECT(SNAP)

#--------------------------------------------------------------------------------
# CMAKE PRELIMINARIES                       
#--------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.12)

IF(POLICY CMP0026)
  cmake_policy(SET CMP0026 OLD)
ENDIF(POLICY CMP0026)

SET(CMAKE_MODULE_PATH ${SNAP_SOURCE_DIR}/CMake)

#--------------------------------------------------------------------------------
# MACROS
#--------------------------------------------------------------------------------
# Get today's date (see http://cmake.3232098.n2.nabble.com/How-to-get-the-current-date-td5776870.html)
MACRO (TODAY RESULT)
    IF (WIN32)
        EXECUTE_PROCESS(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(..)/(..)/(....).*" "\\1/\\2/\\3" ${RESULT} ${${RESULT}})
    ELSEIF(UNIX)
        EXECUTE_PROCESS(COMMAND "date" "+%b %d, %Y" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(...) (..), (....).*" "\\1 \\2, \\3" ${RESULT} ${${RESULT}})
    ELSE (WIN32)
        MESSAGE(SEND_ERROR "date not implemented")
        SET(${RESULT} 000000)
    ENDIF (WIN32)
	string(REPLACE "\n" "" ${RESULT} ${${RESULT}})
	string(REPLACE " " "" ${RESULT} ${${RESULT}})
ENDMACRO (TODAY)

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()


#--------------------------------------------------------------------------------
# VERSION INFORMATION                       
#--------------------------------------------------------------------------------
# On SNAP versions.
# =================
# The SNAP version consists of four fields: major, minor, patch and qualifier
# for example, version 1.7.3-beta has major version 1, minor version 7, patch 3
# and qualifier "-beta". Major, minor and patch must be numbers, but the qualifier
# is an arbitrary string and may be blank. 

# These four fields should be modified when versions change
SET(SNAP_VERSION_MAJOR 3)
SET(SNAP_VERSION_MINOR 4)
SET(SNAP_VERSION_PATCH 0)
SET(SNAP_VERSION_QUALIFIER "-beta")

# These fields should also be modified each time
SET(SNAP_VERSION_RELEASE_DATE "20150807")
SET(SNAP_VERSION_RELEASE_DATE_FORMATTED "Aug 7, 2015")

# This field should only change when the format of the settings files changes
# in a non backwards-compatible way
SET(SNAP_VERSION_LAST_COMPATIBLE_RELEASE_DATE "20131201")

# This should not need to change
SET(SNAP_VERSION_FULL 
  "${SNAP_VERSION_MAJOR}.${SNAP_VERSION_MINOR}.${SNAP_VERSION_PATCH}${SNAP_VERSION_QUALIFIER}")

# Get today's date (see http://cmake.3232098.n2.nabble.com/How-to-get-the-current-date-td5776870.html)
TODAY(SNAP_VERSION_COMPILE_DATE)

# Get the current git hash
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/rpavlik/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC SNAP_VERSION_GIT_SHA1)

# Get the current git branch
include(GitBranch)
get_git_branch(SNAP_VERSION_GIT_BRANCH)

# Print the Git information
MESSAGE(STATUS "   GIT Info: BRANCH ${SNAP_VERSION_GIT_BRANCH}, SHA: ${SNAP_VERSION_GIT_SHA1}")

#--------------------------------------------------------------------------------
# FIND PACKAGES IF BUILDING OUTSIDE INSIGHTAPPLICATIONS 
#--------------------------------------------------------------------------------
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

#--------------------------------------------------------------------------------
# CPACK PACKAGE NAME
# Create a complete name for the package, including the system information
# (Shamelessly stolen from ParaView's CMakeLists.txt)
#--------------------------------------------------------------------------------
# *** THIS CODE MUST APPEAR BEFORE CALLING CONFIGURE_FILE on SNAPCommon.cxx ***
#--------------------------------------------------------------------------------

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
  MESSAGE(STATUS "   INARCH ${CMAKE_OSX_ARCHITECTURES}")

  # TODO: when CMAKE_OSX_ARCHITECTURES is not set this causes an obscure
  # CMake error. Should generate a meaningful message instead
  STRING(REPLACE ";" "-" ARCH ${CMAKE_OSX_ARCHITECTURES})
  SET(CPACK_SYSTEM_NAME "MacOS-${ARCH}")
  MESSAGE(STATUS "   ARCH ${ARCH}")
  MESSAGE(STATUS "   CMAKE_OSX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES}")
ENDIF(CMAKE_SYSTEM_NAME MATCHES Darwin)

MESSAGE(STATUS "   CPACK_SYSTEM_NAME ${CPACK_SYSTEM_NAME}")

SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")

MESSAGE(STATUS "   CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME}")

# Additional CPack resources
SET(CPACK_RESOURCE_FILE_LICENSE "${SNAP_SOURCE_DIR}/COPYING")


#--------------------------------------------------------------------------------
# SOURCE FILE SPECIFICATION                 
#--------------------------------------------------------------------------------

# One of the files needs to be configured (to insert version info)
CONFIGURE_FILE(
  ${SNAP_SOURCE_DIR}/Common/SNAPCommon.cxx.in
  ${SNAP_BINARY_DIR}/SNAPCommon.cxx @ONLY IMMEDIATE)

# Option to use GPU for SNAP
OPTION(SNAP_USE_GPU "Use GPU in SNAP" OFF) 

# Pass the option SNAP_USE_GPU to a header file
CONFIGURE_FILE(
  ${SNAP_SOURCE_DIR}/Common/GPUSettings.h.in
  ${SNAP_BINARY_DIR}/GPUSettings.h @ONLY IMMEDIATE)

# The part of the source code devoted to the SNAP application logic
# is organized into a separate library
SET(LOGIC_CXX
  ${SNAP_BINARY_DIR}/SNAPCommon.cxx
  Common/AbstractModel.cxx
  Common/AbstractPropertyContainerModel.cxx
  Common/CommandLineArgumentParser.cxx
  Common/EventBucket.cxx
  Common/HistoryManager.cxx
  Common/IPCHandler.cxx
  Common/IRISException.cxx
  Common/Rebroadcaster.cxx
  Common/Registry.cxx
  Common/SNAPOpenGL.cxx
  Common/SystemInterface.cxx
  Common/ThreadSpecificData.cxx
  Common/ITKExtras/itkVoxBoCUBImageIO.cxx
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.cxx
  Common/Trackball.cxx
  Logic/Common/ColorLabelTable.cxx
  Logic/Common/ColorMap.cxx
  Logic/Common/ColorMapPresetManager.cxx
  Logic/Common/ImageCoordinateGeometry.cxx
  Logic/Common/ImageCoordinateTransform.cxx
  Logic/Common/IRISDisplayGeometry.cxx
  Logic/Common/LabelUseHistory.cxx
  Logic/Common/MetaDataAccess.cxx
  Logic/Common/SegmentationStatistics.cxx
  Logic/Common/SNAPAppearanceSettings.cxx
  Logic/Common/SNAPRegistryIO.cxx
  Logic/Common/SNAPSegmentationROISettings.cxx
  Logic/Framework/DefaultBehaviorSettings.cxx
  Logic/Framework/GenericImageData.cxx
  Logic/Framework/GlobalState.cxx
  Logic/Framework/ImageAnnotationData.cxx
  Logic/Framework/ImageIODelegates.cxx
  Logic/Framework/IRISApplication.cxx
  Logic/Framework/IRISImageData.cxx
  Logic/Framework/LayerIterator.cxx
  Logic/Framework/SNAPImageData.cxx
  Logic/Framework/UndoDataManager_LabelType.cxx
  Logic/ImageWrapper/CommonRepresentationPolicy.cxx
  Logic/ImageWrapper/DisplayMappingPolicy.cxx
  Logic/ImageWrapper/ImageWrapperBase.cxx
  Logic/ImageWrapper/ImageWrapper.cxx
  Logic/ImageWrapper/InputSelectionImageFilter.cxx
  Logic/ImageWrapper/GuidedNativeImageIO.cxx
  Logic/ImageWrapper/ScalarImageHistogram.cxx
  Logic/ImageWrapper/ScalarImageWrapper.cxx
  Logic/ImageWrapper/VectorImageWrapper.cxx
  Logic/LevelSet/SnakeParameters.cxx
  Logic/LevelSet/SnakeParametersPreviewPipeline.cxx
  Logic/Mesh/AllPurposeProgressAccumulator.cxx
  Logic/Mesh/GuidedMeshIO.cxx
  Logic/Mesh/MultiLabelMeshPipeline.cxx
  Logic/Mesh/LevelSetMeshPipeline.cxx
  Logic/Mesh/MeshManager.cxx
  Logic/Mesh/MeshOptions.cxx
  Logic/Mesh/VTKMeshPipeline.cxx
  Logic/Preprocessing/EdgePreprocessingSettings.cxx
  Logic/Preprocessing/PreprocessingFilterConfigTraits.cxx
  Logic/Preprocessing/ThresholdSettings.cxx
  Logic/Preprocessing/GMM/EMGaussianMixtures.cxx
  Logic/Preprocessing/GMM/Gaussian.cxx
  Logic/Preprocessing/GMM/GaussianMixtureModel.cxx
  Logic/Preprocessing/GMM/KMeansPlusPlus.cxx
  Logic/Preprocessing/GMM/UnsupervisedClustering.cxx
  Logic/Preprocessing/RandomForest/RFClassificationEngine.cxx
  Logic/Preprocessing/RandomForest/RandomForestClassifier.cxx
  Logic/Preprocessing/Texture/MomentTextures.cxx
  Logic/Slicing/IntensityCurveVTK.cxx
  Logic/Slicing/IntensityToColorLookupTableImageFilter.cxx
  Logic/Slicing/LookupTableIntensityMappingFilter.cxx
  Logic/Slicing/RGBALookupTableIntensityMappingFilter.cxx
)

# The headers for the Logic code
SET(LOGIC_HEADERS
  ${SNAP_BINARY_DIR}/GPUSettings.h
  Common/AbstractModel.h
  Common/AbstractPropertyContainerModel.h
  Common/CommandLineArgumentParser.h
  Common/Credits.h
  Common/HistoryManager.h
  Common/IPCHandler.h
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
  Common/PresetManager.h
  Common/PresetManager.hxx
  Common/PropertyModel.h
  Common/Rebroadcaster.h
  Common/Registry.h
  Common/SNAPBorlandDummyTypes.h
  Common/SNAPCommon.h
  Common/SNAPOpenGL.h
  Common/SNAPEvents.h
  Common/SystemInterface.h
  Common/ThreadSpecificData.h
  Common/Trackball.h
  Logic/Common/ColorLabel.h
  Logic/Common/ColorLabelTable.h
  Logic/Common/ColorMap.h
  Logic/Common/ColorMapPresetManager.h
  Logic/Common/ImageCoordinateGeometry.h
  Logic/Common/ImageCoordinateTransform.h
  Logic/Common/IRISDisplayGeometry.h
  Logic/Common/LabelUseHistory.h
  Logic/Common/SegmentationStatistics.h
  Logic/Common/ImageRayIntersectionFinder.h
  Logic/Common/ImageRayIntersectionFinder.txx
  Logic/Common/MetaDataAccess.h
  Logic/Common/SNAPAppearanceSettings.h
  Logic/Common/SNAPRegistryIO.h
  Logic/Common/SNAPSegmentationROISettings.h
  Logic/Framework/DefaultBehaviorSettings.h
  Logic/Framework/GenericImageData.h
  Logic/Framework/GlobalState.h
  Logic/Framework/ImageAnnotationData.h
  Logic/Framework/ImageIODelegates.h
  Logic/Framework/IRISApplication.h
  Logic/Framework/IRISImageData.h
  Logic/Framework/LayerAssociation.h
  Logic/Framework/LayerAssociation.txx
  Logic/Framework/LayerIterator.h
  Logic/Framework/SNAPImageData.h
  Logic/Framework/UndoDataManager.h
  Logic/Framework/UndoDataManager.txx
  Logic/ImageWrapper/CommonRepresentationPolicy.h
  Logic/ImageWrapper/DisplayMappingPolicy.h
  Logic/ImageWrapper/GuidedNativeImageIO.h
  Logic/ImageWrapper/ImageWrapper.h
  Logic/ImageWrapper/ImageWrapperBase.h
  Logic/ImageWrapper/ImageWrapperTraits.h
  Logic/ImageWrapper/InputSelectionImageFilter.h
  Logic/ImageWrapper/LabelToRGBAFilter.h
  Logic/ImageWrapper/NativeIntensityMappingPolicy.h
  Logic/ImageWrapper/ScalarImageHistogram.h
  Logic/ImageWrapper/ScalarImageWrapper.h
  Logic/ImageWrapper/ThreadedHistogramImageFilter.h
  Logic/ImageWrapper/ThreadedHistogramImageFilter.hxx
  Logic/ImageWrapper/VectorImageWrapper.h
  Logic/ImageWrapper/CPUImageToGPUImageFilter.h
  Logic/ImageWrapper/CPUImageToGPUImageFilter.hxx
  Logic/LevelSet/LevelSetExtensionFilter.h
  Logic/LevelSet/SnakeParametersPreviewPipeline.h
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
  Logic/Mesh/MultiLabelMeshPipeline.h
  Logic/Mesh/LevelSetMeshPipeline.h
  Logic/Mesh/MeshManager.h
  Logic/Mesh/MeshOptions.h
  Logic/Mesh/VTKMeshPipeline.h
  Logic/Preprocessing/EdgePreprocessingImageFilter.h
  Logic/Preprocessing/EdgePreprocessingImageFilter.txx
  Logic/Preprocessing/EdgePreprocessingSettings.h
  Logic/Preprocessing/GMMClassifyImageFilter.h
  Logic/Preprocessing/GMMClassifyImageFilter.txx
  Logic/Preprocessing/PreprocessingFilterConfigTraits.h
  Logic/Preprocessing/RandomForestClassifyImageFilter.h
  Logic/Preprocessing/RandomForestClassifyImageFilter.txx
  Logic/Preprocessing/SlicePreviewFilterWrapper.h
  Logic/Preprocessing/SlicePreviewFilterWrapper.txx
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.h
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.txx
  Logic/Preprocessing/ThresholdSettings.h
  Logic/Preprocessing/GMM/EMGaussianMixtures.h
  Logic/Preprocessing/GMM/Gaussian.h
  Logic/Preprocessing/GMM/GaussianMixtureModel.h
  Logic/Preprocessing/GMM/KMeansPlusPlus.h
  Logic/Preprocessing/GMM/UnsupervisedClustering.h
  Logic/Preprocessing/RandomForest/RFClassificationEngine.h
  Logic/Preprocessing/RandomForest/RandomForestClassifier.h
  Logic/Preprocessing/Texture/MomentTextures.h
  Logic/Slicing/IRISSlicer.h
  Logic/Slicing/IRISSlicer.txx
  Logic/Slicing/IntensityCurveInterface.h
  Logic/Slicing/IntensityCurveVTK.h
  Logic/Slicing/IntensityToColorLookupTableImageFilter.h
  Logic/Slicing/LookupTableIntensityMappingFilter.h
  Logic/Slicing/RGBALookupTableIntensityMappingFilter.h
)

# These files have the UI model code, which is GUI-TK independent
SET(UI_GENERIC_CXX
  GUI/Model/AnnotationModel.cxx
  GUI/Model/ColorLabelQuickListModel.cxx
  GUI/Model/ColorLabelPropertyModel.cxx
  GUI/Model/ColorMapModel.cxx
  GUI/Model/CursorInspectionModel.cxx
  GUI/Model/DisplayLayoutModel.cxx
  GUI/Model/Generic3DModel.cxx
  GUI/Model/GenericSliceModel.cxx
  GUI/Model/GlobalPreferencesModel.cxx
  GUI/Model/GlobalUIModel.cxx
  GUI/Model/ImageIOWizardModel.cxx
  GUI/Model/ImageInfoModel.cxx
  GUI/Model/ImageRegistrationManager.cxx
  GUI/Model/IntensityCurveModel.cxx
  GUI/Model/LabelEditorModel.cxx
  GUI/Model/LayerGeneralPropertiesModel.cxx
  GUI/Model/LayerTableRowModel.cxx
  GUI/Model/LayerSelectionModel.cxx
  GUI/Model/MeshExportModel.cxx
  GUI/Model/OrthogonalSliceCursorNavigationModel.cxx
  GUI/Model/PaintbrushModel.cxx
  GUI/Model/PaintbrushSettingsModel.cxx
  GUI/Model/PolygonDrawingModel.cxx
  GUI/Model/PolygonSettingsModel.cxx
  GUI/Model/ReorientImageModel.cxx
  GUI/Model/SaveModifiedLayersModel.cxx
  GUI/Model/SliceWindowCoordinator.cxx
  GUI/Model/SnakeParameterModel.cxx
  GUI/Model/SnakeROIModel.cxx
  GUI/Model/SnakeROIResampleModel.cxx
  GUI/Model/SnakeWizardModel.cxx
  GUI/Model/StateManagement.cxx
  GUI/Model/SynchronizationModel.cxx
  GUI/Model/UIAction.cxx
  GUI/Renderer/AbstractRenderer.cxx
  GUI/Renderer/AbstractVTKRenderer.cxx
  GUI/Renderer/AbstractVTKSceneRenderer.cxx
  GUI/Renderer/AnnotationRenderer.cxx
  GUI/Renderer/ColorMapRenderer.cxx
  GUI/Renderer/CrosshairsRenderer.cxx
  GUI/Renderer/EdgePreprocessingSettingsRenderer.cxx
  GUI/Renderer/GenericSliceRenderer.cxx
  GUI/Renderer/Generic3DRenderer.cxx
  GUI/Renderer/GLToPNG.cxx
  GUI/Renderer/GMMRenderer.cxx
  GUI/Renderer/IntensityCurveVTKRenderer.cxx
  GUI/Renderer/LayerHistogramPlotAssembly.cxx
  GUI/Renderer/OpenGLSliceTexture.cxx
  GUI/Renderer/OptimizationProgressRenderer.cxx
  GUI/Renderer/OrientationGraphicRenderer.cxx
  GUI/Renderer/PaintbrushRenderer.cxx
  GUI/Renderer/PolygonDrawingRenderer.cxx
  GUI/Renderer/PolygonScanConvert.cxx
  GUI/Renderer/SliceWindowDecorationRenderer.cxx
  GUI/Renderer/SnakeParameterPreviewRenderer.cxx
  GUI/Renderer/SnakeROIRenderer.cxx
  GUI/Renderer/SnakeModeRenderer.cxx
  GUI/Renderer/ThresholdSettingsRenderer.cxx
  GUI/Renderer/Window3DPicker.cxx
  GUI/Renderer/OrientationWidget/Reorient/AbstractScannerHelper.cxx
  GUI/Renderer/OrientationWidget/Reorient/AxesWidget.cxx
  GUI/Renderer/OrientationWidget/Reorient/ScannedHuman.cxx
  GUI/Renderer/OrientationWidget/Reorient/ScanningROI.cxx
  GUI/Renderer/OrientationWidget/Reorient/ReorientProps.cxx
  GUI/Renderer/OrientationWidget/Reorient/PolyDataAlgorithm2ActorPipe.cxx
)

SET(UI_GENERIC_HEADERS
  GUI/Model/AbstractLayerAssociatedModel.h
  GUI/Model/AbstractLayerInfoItemSetDomain.h
  GUI/Model/AnnotationModel.h
  GUI/Model/ColorMapModel.h
  GUI/Model/ColorLabelQuickListModel.h
  GUI/Model/ColorLabelPropertyModel.h
  GUI/Model/CursorInspectionModel.h
  GUI/Model/DisplayLayoutModel.h
  GUI/Model/Generic3DModel.h
  GUI/Model/GenericSliceModel.h
  GUI/Model/GlobalPreferencesModel.h
  GUI/Model/GlobalUIModel.h
  GUI/Model/ImageInfoModel.h
  GUI/Model/ImageIOWizardModel.h
  GUI/Model/ImageRegistrationManager.h
  GUI/Model/IntensityCurveModel.h
  GUI/Model/LabelEditorModel.h
  GUI/Model/LayerGeneralPropertiesModel.h
  GUI/Model/LayerSelectionModel.h
  GUI/Model/LayerTableRowModel.h
  GUI/Model/MeshExportModel.h
  GUI/Model/OrthogonalSliceCursorNavigationModel.h
  GUI/Model/PaintbrushModel.h
  GUI/Model/PaintbrushSettingsModel.h
  GUI/Model/PolygonSettingsModel.h
  GUI/Model/PolygonDrawingModel.h
  GUI/Model/ReorientImageModel.h
  GUI/Model/SaveModifiedLayersModel.h
  GUI/Model/SNAPUIFlag.h
  GUI/Model/SNAPUIFlag.txx
  GUI/Model/SliceWindowCoordinator.h
  GUI/Model/SnakeParameterModel.h
  GUI/Model/SnakeROIModel.h
  GUI/Model/SnakeROIResampleModel.h
  GUI/Model/SnakeWizardModel.h
  GUI/Model/StateManagement.h
  GUI/Model/SynchronizationModel.h
  GUI/Model/UIAction.h
  GUI/Model/UIReporterDelegates.h
  GUI/Model/UIState.h
  GUI/Renderer/AbstractRenderer.h
  GUI/Renderer/AbstractVTKRenderer.h
  GUI/Renderer/AbstractVTKSceneRenderer.h
  GUI/Renderer/AnnotationRenderer.h
  GUI/Renderer/ColorMapRenderer.h
  GUI/Renderer/CrosshairsRenderer.h
  GUI/Renderer/EdgePreprocessingSettingsRenderer.h
  GUI/Renderer/Generic3DRenderer.h
  GUI/Renderer/GenericSliceRenderer.h
  GUI/Renderer/GLToPNG.h
  GUI/Renderer/GMMRenderer.h
  GUI/Renderer/IntensityCurveVTKRenderer.h
  GUI/Renderer/LayerHistogramPlotAssembly.h
  GUI/Renderer/OptimizationProgressRenderer.h
  GUI/Renderer/OrientationGraphicRenderer.h
  GUI/Renderer/PaintbrushRenderer.h
  GUI/Renderer/PolygonDrawingRenderer.h
  GUI/Renderer/PolygonScanConvert.h
  GUI/Renderer/SliceWindowDecorationRenderer.h
  GUI/Renderer/SnakeParameterPreviewRenderer.h
  GUI/Renderer/SnakeROIRenderer.h
  GUI/Renderer/SnakeModeRenderer.h
  GUI/Renderer/ThresholdSettingsRenderer.h
  GUI/Renderer/Window3DPicker.h
  GUI/Renderer/OrientationWidget/Reorient/AbstractScannerHelper.h
  GUI/Renderer/OrientationWidget/Reorient/AxesWidget.h
  GUI/Renderer/OrientationWidget/Reorient/ScannedHuman.h
  GUI/Renderer/OrientationWidget/Reorient/ScanningROI.h
  GUI/Renderer/OrientationWidget/Reorient/ReorientProps.h
  GUI/Renderer/OrientationWidget/Reorient/PolyDataAlgorithm2ActorPipe.h
)

# These files contain the Qt-specific user interface source code
SET(UI_QT_CXX
  GUI/Qt/Components/AnnotationToolPanel.cxx
  GUI/Qt/Components/CollapsableGroupBox.cxx
  GUI/Qt/Components/ColorLabelQuickListWidget.cxx
  GUI/Qt/Components/ColorMapInspector.cxx
  GUI/Qt/Components/ContrastInspector.cxx
  GUI/Qt/Components/CursorInspector.cxx
  GUI/Qt/Components/DisplayLayoutInspector.cxx
  GUI/Qt/Components/FileChooserPanelWithHistory.cxx
  GUI/Qt/Components/HistoryQListModel.cxx
  GUI/Qt/Components/ImageInfoInspector.cxx
  GUI/Qt/Components/LabelInspector.cxx
  GUI/Qt/Components/LabelMiniInspector.cxx
  GUI/Qt/Components/LabelSelectionButton.cxx
  GUI/Qt/Components/LatentITKEventNotifier.cxx
  GUI/Qt/Components/LayerInspectorRowDelegate.cxx
  GUI/Qt/Components/MetadataInspector.cxx
  GUI/Qt/Components/GeneralLayerInspector.cxx
  GUI/Qt/Components/PaintbrushToolPanel.cxx
  GUI/Qt/Components/PolygonToolPanel.cxx
  GUI/Qt/Components/QActionButton.cxx
  GUI/Qt/Components/QColorButtonWidget.cxx
  GUI/Qt/Components/QDoubleSlider.cxx
  GUI/Qt/Components/QDoubleSliderWithEditor.cxx
  GUI/Qt/Components/QtHideOnDeactivateContainer.cxx
  GUI/Qt/Components/QtIPCManager.cxx
  GUI/Qt/Components/QtRendererPlatformSupport.cxx
  GUI/Qt/Components/QtReporterDelegates.cxx
  GUI/Qt/Components/QtWarningDialog.cxx
  GUI/Qt/Components/QtWidgetActivator.cxx
  GUI/Qt/Components/RecentHistoryItemsView.cxx
  GUI/Qt/Components/SnakeToolROIPanel.cxx
  GUI/Qt/Components/SnakeWizardPanel.cxx
  GUI/Qt/Components/SNAPComponent.cxx
  GUI/Qt/Components/SNAPQtCommon.cxx
  GUI/Qt/Components/SliceViewPanel.cxx
  GUI/Qt/Components/SynchronizationInspector.cxx
  GUI/Qt/Components/ViewPanel3D.cxx
  GUI/Qt/Components/VoxelIntensityQTableModel.cxx
  GUI/Qt/Components/ZoomInspector.cxx
  GUI/Qt/External/ColorWheel/ColorWheel.cxx
  GUI/Qt/ModelView/GMMTableModel.cxx
  GUI/Qt/View/AnnotationInteractionMode.cxx
  GUI/Qt/View/ColorMapBox.cxx
  GUI/Qt/View/CrosshairsInteractionMode.cxx
  GUI/Qt/View/GenericSliceView.cxx
  GUI/Qt/View/GenericView3D.cxx
  GUI/Qt/View/InteractionModeClient.cxx
  GUI/Qt/View/QtAbstractOpenGLBox.cxx
  GUI/Qt/View/QtInteractionDelegateWidget.cxx
  GUI/Qt/View/QtSimpleOpenGLBox.cxx
  GUI/Qt/View/QtVTKInteractionDelegateWidget.cxx
  GUI/Qt/View/QtVTKRenderWindowBox.cxx
  GUI/Qt/View/PaintbrushInteractionMode.cxx
  GUI/Qt/View/PolygonDrawingInteractionMode.cxx
  GUI/Qt/View/SliceWindowInteractionDelegateWidget.cxx
  GUI/Qt/View/SnakeROIInteractionMode.cxx
  GUI/Qt/View/ThumbnailInteractionMode.cxx
  GUI/Qt/Windows/AboutDialog.cxx
  GUI/Qt/Windows/DropActionDialog.cxx
  GUI/Qt/Windows/ImageIODialog.cxx
  GUI/Qt/Windows/ImageIOWizard.cxx
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.cxx
  GUI/Qt/Windows/ImageIOWizard/RegistrationPage.cxx
  GUI/Qt/Windows/LabelEditorDialog.cxx
  GUI/Qt/Windows/LayerInspectorDialog.cxx
  GUI/Qt/Windows/LabelSelectionPopup.cxx
  GUI/Qt/Windows/MainControlPanel.cxx
  GUI/Qt/Windows/MainImageWindow.cxx
  GUI/Qt/Windows/PreferencesDialog.cxx
  GUI/Qt/Windows/QtStyles.cxx
  GUI/Qt/Windows/ReorientImageDialog.cxx
  GUI/Qt/Windows/ResampleDialog.cxx
  GUI/Qt/Windows/SaveModifiedLayersDialog.cxx
  GUI/Qt/Windows/SimpleFileDialogWithHistory.cxx
  GUI/Qt/Windows/SnakeParameterDialog.cxx
  GUI/Qt/Windows/SpeedImageDialog.cxx
  GUI/Qt/Windows/SplashPanel.cxx
  GUI/Qt/Windows/StatisticsDialog.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.cxx
  Testing/GUI/Qt/SNAPTestQt.cxx
)

# The header files for the UI project
SET(UI_MOC_HEADERS
  GUI/Qt/Components/AnnotationToolPanel.h
  GUI/Qt/Components/CollapsableGroupBox.h
  GUI/Qt/Components/ColorLabelQuickListWidget.h
  GUI/Qt/Components/ColorMapInspector.h
  GUI/Qt/Components/ContrastInspector.h
  GUI/Qt/Components/CursorInspector.h
  GUI/Qt/Components/DisplayLayoutInspector.h
  GUI/Qt/Components/FileChooserPanelWithHistory.h
  GUI/Qt/Components/HistoryQListModel.h
  GUI/Qt/Components/ImageInfoInspector.h
  GUI/Qt/Components/LabelInspector.h
  GUI/Qt/Components/LabelMiniInspector.h
  GUI/Qt/Components/LabelSelectionButton.h
  GUI/Qt/Components/LatentITKEventNotifier.h
  GUI/Qt/Components/LayerInspectorRowDelegate.h
  GUI/Qt/Components/MetadataInspector.h
  GUI/Qt/Components/GeneralLayerInspector.h
  GUI/Qt/Components/PaintbrushToolPanel.h
  GUI/Qt/Components/PolygonToolPanel.h
  GUI/Qt/Components/QActionButton.h
  GUI/Qt/Components/QColorButtonWidget.h
  GUI/Qt/Components/QDoubleSlider.h
  GUI/Qt/Components/QDoubleSliderWithEditor.h
  GUI/Qt/Components/QtHideOnDeactivateContainer.h
  GUI/Qt/Components/QtIPCManager.h
  GUI/Qt/Components/QtWarningDialog.h
  GUI/Qt/Components/QtWidgetActivator.h
  GUI/Qt/Components/RecentHistoryItemsView.h
  GUI/Qt/Components/SnakeToolROIPanel.h
  GUI/Qt/Components/SnakeWizardPanel.h
  GUI/Qt/Components/SNAPComponent.h
  GUI/Qt/Components/SliceViewPanel.h
  GUI/Qt/Components/SynchronizationInspector.h
  GUI/Qt/Components/ViewPanel3D.h
  GUI/Qt/Components/VoxelIntensityQTableModel.h
  GUI/Qt/Components/ZoomInspector.h
  GUI/Qt/Coupling/QtWidgetCoupling.h
  GUI/Qt/External/ColorWheel/ColorWheel.h
  GUI/Qt/ModelView/GMMTableModel.h
  GUI/Qt/View/AnnotationInteractionMode.h
  GUI/Qt/View/ColorMapBox.h
  GUI/Qt/View/CrosshairsInteractionMode.h
  GUI/Qt/View/GenericSliceView.h
  GUI/Qt/View/GenericView3D.h
  GUI/Qt/View/QtAbstractOpenGLBox.h
  GUI/Qt/View/QtInteractionDelegateWidget.h
  GUI/Qt/View/QtSimpleOpenGLBox.h
  GUI/Qt/View/QtVTKInteractionDelegateWidget.h
  GUI/Qt/View/QtVTKRenderWindowBox.h
  GUI/Qt/View/PaintbrushInteractionMode.h
  GUI/Qt/View/PolygonDrawingInteractionMode.h
  GUI/Qt/View/SliceWindowInteractionDelegateWidget.h
  GUI/Qt/View/SnakeROIInteractionMode.h
  GUI/Qt/View/ThumbnailInteractionMode.h
  GUI/Qt/Windows/AboutDialog.h
  GUI/Qt/Windows/DropActionDialog.h
  GUI/Qt/Windows/ImageIODialog.h
  GUI/Qt/Windows/ImageIOWizard.h
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.h
  GUI/Qt/Windows/ImageIOWizard/RegistrationPage.h
  GUI/Qt/Windows/LabelEditorDialog.h
  GUI/Qt/Windows/LayerInspectorDialog.h
  GUI/Qt/Windows/LabelSelectionPopup.h
  GUI/Qt/Windows/MainControlPanel.h
  GUI/Qt/Windows/MainImageWindow.h
  GUI/Qt/Windows/PreferencesDialog.h
  GUI/Qt/Windows/ReorientImageDialog.h
  GUI/Qt/Windows/ResampleDialog.h
  GUI/Qt/Windows/SaveModifiedLayersDialog.h
  GUI/Qt/Windows/SimpleFileDialogWithHistory.h
  GUI/Qt/Windows/SnakeParameterDialog.h
  GUI/Qt/Windows/SpeedImageDialog.h
  GUI/Qt/Windows/SplashPanel.h
  GUI/Qt/Windows/StatisticsDialog.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.h
  Testing/GUI/Qt/SNAPTestQt.h
)

# These UI headers don't need to be MOC'd
SET(UI_NONMOC_HEADERS
  GUI/Qt/Components/SNAPQtCommon.h
  GUI/Qt/Components/QtCursorOverride.h
  GUI/Qt/Components/QtRendererPlatformSupport.h
  GUI/Qt/Components/QtReporterDelegates.h
  GUI/Qt/Coupling/QtAbstractButtonCoupling.h
  GUI/Qt/Coupling/QtAbstractItemViewCoupling.h
  GUI/Qt/Coupling/QtActionCoupling.h
  GUI/Qt/Coupling/QtCheckBoxCoupling.h
  GUI/Qt/Coupling/QtColorWheelCoupling.h
  GUI/Qt/Coupling/QtComboBoxCoupling.h
  GUI/Qt/Coupling/QtDoubleSliderWithEditorCoupling.h
  GUI/Qt/Coupling/QtDoubleSpinBoxCoupling.h
  GUI/Qt/Coupling/QtLabelCoupling.h
  GUI/Qt/Coupling/QtLineEditCoupling.h
  GUI/Qt/Coupling/QtListWidgetCoupling.h
  GUI/Qt/Coupling/QtRadioButtonCoupling.h
  GUI/Qt/Coupling/QtScrollbarCoupling.h
  GUI/Qt/Coupling/QtSliderCoupling.h
  GUI/Qt/Coupling/QtSpinBoxCoupling.h
  GUI/Qt/Coupling/QtTableWidgetCoupling.h
  GUI/Qt/Coupling/QtWidgetArrayCoupling.h
  GUI/Qt/Coupling/QtWidgetCoupling.h
)

SET(UI_FORMS
  GUI/Qt/Components/AnnotationToolPanel.ui
  GUI/Qt/Components/CollapsableGroupBox.ui
  GUI/Qt/Components/ColorMapInspector.ui
  GUI/Qt/Components/ContrastInspector.ui
  GUI/Qt/Components/CursorInspector.ui
  GUI/Qt/Components/DisplayLayoutInspector.ui
  GUI/Qt/Components/FileChooserPanelWithHistory.ui
  GUI/Qt/Components/ImageInfoInspector.ui
  GUI/Qt/Components/LabelInspector.ui
  GUI/Qt/Components/LabelMiniInspector.ui
  GUI/Qt/Components/LayerInspectorRowDelegate.ui
  GUI/Qt/Components/MetadataInspector.ui
  GUI/Qt/Components/GeneralLayerInspector.ui
  GUI/Qt/Components/PaintbrushToolPanel.ui
  GUI/Qt/Components/PolygonToolPanel.ui
  GUI/Qt/Components/QDoubleSliderWithEditor.ui
  GUI/Qt/Components/QtWarningDialog.ui
  GUI/Qt/Components/RecentHistoryItemsView.ui
  GUI/Qt/Components/SliceViewPanel.ui
  GUI/Qt/Components/SnakeToolROIPanel.ui
  GUI/Qt/Components/SnakeWizardPanel.ui
  GUI/Qt/Components/SynchronizationInspector.ui
  GUI/Qt/Components/ViewPanel3D.ui
  GUI/Qt/Components/ZoomInspector.ui
  GUI/Qt/Windows/AboutDialog.ui
  GUI/Qt/Windows/DropActionDialog.ui
  GUI/Qt/Windows/ImageIODialog.ui
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.ui
  GUI/Qt/Windows/ImageIOWizard/RegistrationPage.ui
  GUI/Qt/Windows/LabelEditorDialog.ui
  GUI/Qt/Windows/LayerInspectorDialog.ui
  GUI/Qt/Windows/LabelSelectionPopup.ui
  GUI/Qt/Windows/MainControlPanel.ui
  GUI/Qt/Windows/MainImageWindow.ui
  GUI/Qt/Windows/PreferencesDialog.ui
  GUI/Qt/Windows/ReorientImageDialog.ui
  GUI/Qt/Windows/ResampleDialog.ui
  GUI/Qt/Windows/SaveModifiedLayersDialog.ui
  GUI/Qt/Windows/SimpleFileDialogWithHistory.ui
  GUI/Qt/Windows/SnakeParameterDialog.ui
  GUI/Qt/Windows/SpeedImageDialog.ui
  GUI/Qt/Windows/SplashPanel.ui
  GUI/Qt/Windows/StatisticsDialog.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.ui
)

SET(UI_RESOURCES
  GUI/Qt/Resources/SNAPResources.qrc
  GUI/Qt/Resources/SNAPResources_Linux.qrc
  GUI/Qt/Resources/SNAPResources_MacOS.qrc
  GUI/Qt/Resources/SNAPResources_Windows.qrc
  Testing/GUI/Qt/TestingScripts.qrc
)

# The source code for SNAP testing project
SET(TESTING_CXX
  Testing/Logic/TestMain.cxx
  Testing/Logic/SNAPTestDriver.cxx
)

# The source code for the tutorial test
SET(TESTING_TUTORIAL_CXX
  Testing/Logic/TutorialTest.cxx
)

# The headers for the testing code
SET(TESTING_HEADERS
  Testing/Logic/SNAPTestDriver.h
  Testing/Logic/TestBase.h
  Testing/Logic/TestCompareLevelSets.h
  Testing/Logic/TestImageWrapper.h
)

#--------------------------------------------------------------------------------
# Specify include path
#--------------------------------------------------------------------------------

# Due to a limitation in Visual studio 6.0 on the length of include directories
# that can be specified, (here we are including all the include directories from
# ITK, VTK, FLTK and SNAP), if the compiler is VS6, we copy the SNAP source files
# to a single path in the binary tree to cut down on the number of
# INCLUDE_DIRECTORIES
IF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_CXX "${SNAP_SOURCE_DIR}/*.cxx" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_H "${SNAP_SOURCE_DIR}/*.h" )
  FILE( GLOB_RECURSE SNAP_GLOBBED_TXX "${SNAP_SOURCE_DIR}/*.txx" )
  SET(SNAP_SOURCES ${SNAP_GLOBBED_CXX} ${SNAP_GLOBBED_H} ${SNAP_GLOBBED_TXX})
  MAKE_DIRECTORY( "${SNAP_BINARY_DIR}/src" )
  SET( CONFIGURED_SOURCE_DIRECTORY "${SNAP_BINARY_DIR}/src" )
  FOREACH( SourceFile ${SNAP_SOURCES} )
    GET_FILENAME_COMPONENT( CONFIGURED_SOURCE_FILE ${SourceFile} NAME )
    SET( CONFIGURED_SOURCE_FILE "${CONFIGURED_SOURCE_DIRECTORY}/${CONFIGURED_SOURCE_FILE}" )
    CONFIGURE_FILE( ${SourceFile} ${CONFIGURED_SOURCE_FILE} COPYONLY IMMEDIATE )
  ENDFOREACH( SourceFile )
  INCLUDE_DIRECTORIES(
    ${CONFIGURED_SOURCE_DIRECTORY}
    ${ITK_DIR}/Utilities/zlib
    ${OPENGL_INCLUDE_PATH}
    ${QT_QTSCRIPT_INCLUDE_DIR}
    ${QT_QTSCRIPTTOOLS_INCLUDE_DIR})

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
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing/GMM
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing/RandomForest
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing/Texture
    ${SNAP_SOURCE_DIR}/Logic/Slicing
    ${SNAP_SOURCE_DIR}/GUI
    ${SNAP_SOURCE_DIR}/GUI/Model
    ${SNAP_SOURCE_DIR}/GUI/Renderer
    ${SNAP_SOURCE_DIR}/GUI/Renderer/OrientationWidget/Reorient
    ${SNAP_SOURCE_DIR}/GUI/Qt
    ${SNAP_SOURCE_DIR}/GUI/Qt/Components
    ${SNAP_SOURCE_DIR}/GUI/Qt/Coupling
    ${SNAP_SOURCE_DIR}/GUI/Qt/External
    ${SNAP_SOURCE_DIR}/GUI/Qt/External/ColorWheel
    ${SNAP_SOURCE_DIR}/GUI/Qt/ModelView
    ${SNAP_SOURCE_DIR}/GUI/Qt/Testing
    ${SNAP_SOURCE_DIR}/GUI/Qt/View
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows/MeshExportWizard
    ${SNAP_SOURCE_DIR}/Testing/Logic
    ${SNAP_SOURCE_DIR}/Testing/GUI/Qt
    ${SNAP_BINARY_DIR}
    ${OPENGL_INCLUDE_PATH}
    ${SNAP_QT5_INCLUDE_DIRS}
  )

ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )

#--------------------------------------------------------------------------------
# Compiler-specific warinings
#--------------------------------------------------------------------------------

# Get rid of this ridiculous warning in VS8
IF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9" OR CMAKE_GENERATOR MATCHES "Visual Studio 10" )
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9" OR CMAKE_GENERATOR MATCHES "Visual Studio 10" )
  
IF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )
  ADD_DEFINITIONS(-DNOMINMAX)
ENDIF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )

#--------------------------------------------------------------------------------
# Define External Libraries
#--------------------------------------------------------------------------------

# ITK Libraries
SET(SNAP_ITK_LIBS ${ITK_LIBRARIES})

# Core VTK libraries
SET(SNAP_VTK_LIBS
  vtkChartsCore
  vtkCommonCore
  vtkRenderingCore
  vtkRenderingFreeType
  vtkRenderingFreeTypeOpenGL
  vtkRenderingOpenGL
  vtkRenderingVolume
  vtkRenderingVolumeOpenGL
  vtkFiltersCore
  vtkImagingCore
  vtkViewsCore
  vtkViewsContext2D
  vtkIOCore
  vtkIOExport
  vtkIOGeometry
  vtkIOLegacy
)

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
  ${SNAP_SYSTEM_LIBS}
)

#--------------------------------------------------------------------------------
# Define SNAP library targets (logic and UI)
#--------------------------------------------------------------------------------

# Wrap the QT input files
QT5_WRAP_UI(UI_FORM_HEADERS ${UI_FORMS})
QT5_WRAP_CPP(UI_WRAPPED_MOC_HEADERS ${UI_MOC_HEADERS})
QT5_ADD_RESOURCES(UI_RESOURCES_RCC ${UI_RESOURCES})

# The SNAP logic library
ADD_LIBRARY(itksnaplogic ${LOGIC_CXX} ${LOGIC_HEADERS})

# The UI model library
ADD_LIBRARY(itksnapui_model ${UI_GENERIC_CXX} ${UI_GENERIC_HEADERS})

# The user interface code library
ADD_LIBRARY(itksnapui_qt
  ${UI_QT_CXX} ${UI_WRAPPED_MOC_HEADERS} ${UI_MOC_HEADERS} 
  ${UI_NONMOC_HEADERS} ${UI_FORM_HEADERS} ${UI_RESOURCES_RCC})

# This is experimental: it seems that shared libraries do not
# build accurately (at least on MacOS) without the following
# two lines
TARGET_LINK_LIBRARIES(itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui_model itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui_qt ${SNAP_QT5_LIBRARIES} ${SNAP_EXTERNAL_LIBS})

# Designate the SNAP internal libraries
SET(SNAP_INTERNAL_LIBS itksnapui_qt itksnapui_model itksnaplogic)

#--------------------------------------------------------------------------------
# Define main SNAP executable
#--------------------------------------------------------------------------------

# Code for the main application
SET(SNAP_MAIN_SRC GUI/Qt/main.cxx)

# On Apple, configure the application icon
IF(APPLE)

  # the icon file
  SET(SNAP_OSX_ICON ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap.icns)

  # set how it shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE itksnap.icns) 
  
  # set where in the bundle to put the icns file
  SET_SOURCE_FILES_PROPERTIES(${SNAP_OSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  # include the icns file in the target
  SET(SNAP_MAIN_SRC ${SNAP_MAIN_SRC} ${SNAP_OSX_ICON})

ENDIF(APPLE)

# On Windows, add the .RC file to the sources
IF(WIN32)

  # Add the .rc file
  SET(SNAP_MAIN_SRC ${SNAP_MAIN_SRC} ${SNAP_SOURCE_DIR}/Utilities/Win32/itksnap.rc)

ENDIF(WIN32)


# Define the main SNAP executable
SET(SNAP_BUNDLE_NAME ITK-SNAP)

# Configure the executable's sources and libraries
ADD_EXECUTABLE(${SNAP_BUNDLE_NAME} WIN32 MACOSX_BUNDLE ${SNAP_MAIN_SRC})
TARGET_LINK_LIBRARIES(${SNAP_BUNDLE_NAME} ${SNAP_INTERNAL_LIBS} ${SNAP_EXTERNAL_LIBS})

# On apple, we also need our custom plist.info file to be configured
IF(APPLE)
  SET_TARGET_PROPERTIES(${SNAP_BUNDLE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
    ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/Info.plist)
ENDIF(APPLE)

#--------------------------------------------------------------------------------
# Testing-related executables
#--------------------------------------------------------------------------------

ENABLE_TESTING()

include(BundleUtilities)

# The list of Qt animated GUI tests
SET(GUI_TESTS
  Workspace
  ProbeIntensity
  RegionCompetition
  RandomForest)

SET(TESTDATA_DIR "${SNAP_SOURCE_DIR}/Testing/TestData")

# Set up a test for each GUI test
FOREACH(GUI_TEST ${GUI_TESTS})

  # The path of the test - not obvious?
  IF(APPLE)
    GET_PROPERTY(GUI_TEST_EXE TARGET ${SNAP_BUNDLE_NAME} PROPERTY LOCATION)
  ELSE(APPLE)
    SET(GUI_TEST_EXE ${SNAP_BUNDLE_NAME})
  ENDIF(APPLE)
    
  # Add the test
  ADD_TEST(${GUI_TEST} ${GUI_TEST_EXE} --test ${GUI_TEST} --testdir "${TESTDATA_DIR}")

  # Set the environment for the test to include Qt
  FILE(TO_NATIVE_PATH ${QT_BINARY_DIR} QT_BINARY_DIR_NATIVE)

  IF(WIN32)
    SET_PROPERTY(TEST ${GUI_TEST} APPEND PROPERTY ENVIRONMENT PATH=${QT_BINARY_DIR_NATIVE})
  ELSE(WIN32)
    SET_PROPERTY(TEST ${GUI_TEST} APPEND PROPERTY ENVIRONMENT PATH="${QT_BINARY_DIR_NATIVE}:$ENV{PATH}")
  ENDIF(WIN32)
  SET_PROPERTY(TEST ${GUI_TEST} PROPERTY TIMEOUT 120)

ENDFOREACH(GUI_TEST)

# Orientation widget test
SET(TEST_ORIENTATION_WIDGET_HEADERS
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h
)

SET(TEST_ORIENTATION_WIDGET_CXX
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.cxx
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/main.cxx
)

QT5_WRAP_UI(UI_ORIENTATION_WIDGET_SRCS 
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.ui)
QT5_WRAP_CPP(MOC_ORIENTATION_WIDGET_SRCS 
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h)

ADD_EXECUTABLE(Test_OrientationWidget
  ${TEST_ORIENTATION_WIDGET_CXX} ${TEST_ORIENTATION_WIDGET_HEADERS}
  ${UI_ORIENTATION_WIDGET_SRCS} ${MOC_ORIENTATION_WIDGET_SRCS}
)

TARGET_LINK_LIBRARIES(Test_OrientationWidget
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS}
)

INCLUDE(CTest)

#--------------------------------------------------------------------------------
# Copy and configure the program data directory
#--------------------------------------------------------------------------------

# All program files - use recursive globbing
FILE(GLOB_RECURSE SNAP_PROGRAM_DATA_FILES 
  ProgramData "*.txt" "*.html" "*.gif" "*.png" "*.img.gz" "*.hdr")

# Copy documentation from the source tree to the build tree
FOREACH(DATAFILE ${SNAP_PROGRAM_DATA_FILES})
  FILE(RELATIVE_PATH SHORTNAME ${SNAP_SOURCE_DIR} ${DATAFILE})
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/${SHORTNAME}
    ${SNAP_BINARY_DIR}/${SHORTNAME}
   COPYONLY)
ENDFOREACH(DATAFILE)

#--------------------------------------------------------------------------------
# INSTALLATION AND PACKAGING SECTION
#--------------------------------------------------------------------------------

MESSAGE(STATUS "=== Installation Section ===")
MESSAGE(STATUS "   CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE(STATUS "   CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
MESSAGE(STATUS "   PACKAGE_DIR ${CPACK_PACKAGE_DIRECTORY}")
MESSAGE(STATUS "   INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "   DESTDIR $ENV{DESTDIR}")

#--------------------------------------------------------------------------------
# Install the application

IF(APPLE)

  # on Apple, the bundle is at the root of the
  # install tree, and on other platforms it'll go into the bin directory.
  INSTALL(TARGETS ${SNAP_BUNDLE_NAME} 
      BUNDLE DESTINATION . COMPONENT Runtime)

  # On apple, we put a binary script along with a README file
  INSTALL(FILES
          ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap
          DESTINATION . COMPONENT Runtime
          PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                      GROUP_EXECUTE GROUP_READ)

  # Try adding a link to /usr/local/bin
  INSTALL(CODE "EXECUTE_PROCESS(COMMAND ln -sf /usr/local/bin usr_local_bin WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX})")

  INSTALL(FILES
          ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/README.txt
          DESTINATION . COMPONENT Runtime)

  #INSTALL(FILES
  #        ${SNAP_BINARY_DIR}/usr_local_bin
  #        DESTINATION . COMPONENT Runtime)

  # Set the DS STORE
  SET(CPACK_DMG_DS_STORE ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/dsstore.bin)
  SET(CPACK_DMG_BACKGROUND_IMAGE ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/background.png)

  # Include the qt4 dependent libraries
  include(DeployQt5)

  # Make sure the GIF plugin is included
  get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

  # Install with the plugin
  install_qt5_executable(${SNAP_BUNDLE_NAME}.app "${QT_GIF_PLUGIN}")

ELSEIF(WIN32)

  # Install to the bin directory
  INSTALL(TARGETS ${SNAP_BUNDLE_NAME} RUNTIME DESTINATION bin)

  # Include the qt4 dependent libraries
  include(DeployQt5)

  # Make sure the GIF plugin is included
  get_property(QT_WIN_PLUGIN TARGET Qt5::QWindowsIntegrationPlugin PROPERTY LOCATION_RELEASE)
  get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

  # Install with the plugin
  install_qt5_executable(bin/${SNAP_BUNDLE_NAME}.exe "${QT_GIF_PLUGIN};${QT_WIN_PLUGIN}")

  # On windows, we have to configure NSIS
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "ITK-SNAP.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ITK-SNAP")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.itksnap.org")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.itksnap.org/credits.php")
  SET(CPACK_NSIS_MODIFY_PATH OFF)

  # CMake does not yet know to install into (x64) program files or not
  IF(CMAKE_CL_64)
    SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  ENDIF(CMAKE_CL_64)

  # Give it a windowsy directory name
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "ITK-SNAP ${SNAP_VERSION_MAJOR}.${SNAP_VERSION_MINOR}")
  
  # On Win32, the executable is the actual exe
  SET(CPACK_PACKAGE_EXECUTABLES ITK-SNAP "ITK-SNAP")

  # On Win32, we must include the redistributable
  IF(MSVC_VERSION GREATER 1399)
    FIND_PROGRAM(VCREDIST_EXE vcredist_x86.exe vcredist_x64.exe)
    IF(VCREDIST_EXE)
      GET_FILENAME_COMPONENT(VCREDIST_NAME ${VCREDIST_EXE} NAME)
      INSTALL(FILES ${VCREDIST_EXE} DESTINATION bin)
      SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS 
        "ExecWait '\\\"$INSTDIR\\\\bin\\\\${VCREDIST_NAME}\\\" /passive'")
    ENDIF(VCREDIST_EXE)
  ENDIF(MSVC_VERSION GREATER 1399)

ELSE()

  # On Linux, we generate forward shared executable
  SUBDIRS(Utilities/Forwarding)

  SET(SNAP_EXE "ITK-SNAP")
  SET(SNAP_MAIN_INSTALL_DIR lib/snap-${SNAP_VERSION_FULL})
  SET(SNAP_DATA_INSTALL_DIR ${SNAP_MAIN_INSTALL_DIR})
  SET(CPACK_PACKAGE_EXECUTABLES "itksnap" "ITK-SNAP")
  INSTALL(TARGETS ${SNAP_BUNDLE_NAME} RUNTIME DESTINATION ${SNAP_MAIN_INSTALL_DIR})

  # Include the qt4 dependent libraries
  include(DeployQt5)

  # Add an option to not package QT plugins. This is needed for some special builds,
  # for example on Debian
  OPTION(SNAP_PACKAGE_QT_PLUGINS "Try to package Qt5 plugins with the executable" ON)
  MARK_AS_ADVANCED(SNAP_PACKAGE_QT_PLUGINS)

  # Get the necessary plugins
  IF(SNAP_PACKAGE_QT_PLUGINS)

    get_property(QT_XCB_PLUGIN TARGET Qt5::QXcbIntegrationPlugin PROPERTY LOCATION_RELEASE)
    get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

    # Install the plugin
    install_qt5_executable(${SNAP_MAIN_INSTALL_DIR}/${SNAP_EXE} "${QT_XCB_PLUGIN};${QT_GIF_PLUGIN}")

  ELSE(SNAP_PACKAGE_QT_PLUGINS)

    # Install the exe without plugins
    install_qt5_executable(${SNAP_MAIN_INSTALL_DIR}/${SNAP_EXE})

  ENDIF(SNAP_PACKAGE_QT_PLUGINS)

ENDIF()


#--------------------------------------------------------------------------------
# Configure CPack

# Which generator to use
IF(APPLE)
  SET(CPACK_GENERATOR DragNDrop)
  SET(CPACK_EXTENSION "dmg")
ELSEIF(WIN32)
  SET(CPACK_GENERATOR NSIS)
  SET(CPACK_EXTENSION "exe")
ELSE(APPLE)
  SET(CPACK_GENERATOR TGZ)
  SET(CPACK_EXTENSION "tar.gz")
ENDIF(APPLE)

#--------------------------------------------------------------------------------
# Construct the name of the package
SET(CPACK_PACKAGE_FILE_NAME_WEXT "${CPACK_PACKAGE_FILE_NAME}.${CPACK_EXTENSION}")


#--------------------------------------------------------------------------------
# Set up package target
include(CPack)

#--------------------------------------------------------------------------------
# Uploading code to SourceForge
#--------------------------------------------------------------------------------

#--------------------------------------------------------------------------------
# Configure SCP

FIND_PROGRAM(SCP_PROGRAM NAMES scp DOC "Location of the scp program (optional)")
MARK_AS_ADVANCED(SCP_PROGRAM)

SET(SCP_ARGUMENTS "-v" CACHE STRING "Optional arguments to the scp command for uploads to SourceForge")
MARK_AS_ADVANCED(SCP_ARGUMENTS)

SET(SCP_USERNAME "" CACHE STRING "SourceForge.net account id for uploads")
MARK_AS_ADVANCED(SCP_USERNAME)

SET(NIGHTLY_TARGET "itksnap-nightly-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")
SET(EXPERIMENTAL_TARGET "itksnap-experimental-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")

SET(SCP_ROOT "frs.sourceforge.net:/home/frs/project/i/it/itk-snap/itk-snap")

#--------------------------------------------------------------------------------
# Create targets

ADD_CUSTOM_TARGET(upload_nightly 
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS}
  ${CPACK_PACKAGE_FILE_NAME_WEXT} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Nightly/${NIGHTLY_TARGET}
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_PACKAGE_FILE_NAME_WEXT} to SourceForge.net as ${NIGHTLY_TARGET}")

ADD_CUSTOM_TARGET(upload_experimental 
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS} 
    ${CPACK_PACKAGE_FILE_NAME_WEXT} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Experimental/${EXPERIMENTAL_TARGET}
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_PACKAGE_FILE_NAME_WEXT} to SourceForge.net as ${EXPERIMENTAL_TARGET}")


