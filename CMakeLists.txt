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
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# This macro is used to install binary targets.
MACRO(SNAP_INSTALL_GUI NAME CLI_NAME CLI_OPTS)

  IF(APPLE)

    # Install the actual executable
    INSTALL(TARGETS ${NAME} DESTINATION ../MacOS COMPONENT Runtime)

    # Create a caller .sh
    SET(EXENAME "MacOS/${NAME}")
    SET(EXEOPTS "${CLI_OPTS}")
    CONFIGURE_FILE(${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/execaller.sh ${SNAP_BINARY_DIR}/MacOS/Scripts/${CLI_NAME} @ONLY)
    INSTALL(FILES
            ${SNAP_BINARY_DIR}/MacOS/Scripts/${CLI_NAME}
            DESTINATION ../bin COMPONENT Runtime
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                        GROUP_EXECUTE GROUP_READ)
  ELSE()

    INSTALL(TARGETS ${NAME} DESTINATION bin COMPONENT Runtime)

  ENDIF()

ENDMACRO()

# Install path for command-line tools
IF(APPLE)
  SET(SNAP_CLI_INSTALL_PATH "../bin")
ELSE()
  SET(SNAP_CLI_INSTALL_PATH "bin")
ENDIF()

# The commented out code below is useful for debugging, it prints all variables in cache
### get_cmake_property(_variableNames VARIABLES)
### foreach (_variableName ${_variableNames})
###     message(STATUS "${_variableName}=${${_variableName}}")
### endforeach()


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
SET(SNAP_VERSION_MINOR 8)
SET(SNAP_VERSION_PATCH 2)
SET(SNAP_VERSION_QUALIFIER "")

# These fields should also be modified each time
SET(SNAP_VERSION_RELEASE_DATE "20210219")
SET(SNAP_VERSION_RELEASE_DATE_FORMATTED "Feb 19, 2021")

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
get_git_commit_date(${SNAP_VERSION_GIT_BRANCH} SNAP_VERSION_GIT_TIMESTAMP)

# Print the Git information
MESSAGE(STATUS "ITK-SNAP Git Info:")
MESSAGE(STATUS "  Branch : ${SNAP_VERSION_GIT_BRANCH}")
MESSAGE(STATUS "  SHA    : ${SNAP_VERSION_GIT_SHA1}")
MESSAGE(STATUS "  Date   : ${SNAP_VERSION_GIT_TIMESTAMP}")

#--------------------------------------------------------------------------------
# ENSURE THAT SUBMODULES HAVE BEEN INITIALIZED AND UPDATED
#--------------------------------------------------------------------------------
if(NOT EXISTS "${SNAP_SOURCE_DIR}/Submodules/c3d/CMakeLists.txt")
  MESSAGE(SEND_ERROR "Submodule c3d has not been initialized/updated. Git users, see README.git")
endif()

if(NOT EXISTS "${SNAP_SOURCE_DIR}/Submodules/greedy/CMakeLists.txt")
  MESSAGE(SEND_ERROR "Submodule greedy has not been initialized/updated. Git users, see README.git")
endif()

#--------------------------------------------------------------------------------
# FIND PACKAGES IF BUILDING OUTSIDE INSIGHTAPPLICATIONS 
#--------------------------------------------------------------------------------

# Add option to build on qt 4.x
OPTION(SNAP_USE_QT4 "Compile using Qt version 4.8 for compatibility with older systems and VNC/x2go" OFF)

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
IF(CMAKE_SYSTEM_NAME MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)
  LIST(LENGTH CMAKE_OSX_ARCHITECTURES _length)
  IF(_length GREATER 1)
    SET(CPACK_SYSTEM_NAME Darwin-Universal)
  ELSE()
    SET(CPACK_SYSTEM_NAME Darwin-${CMAKE_OSX_ARCHITECTURES})
  ENDIF()
ENDIF()

IF(NOT SNAP_USE_QT4)
  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
ELSE(NOT SNAP_USE_QT4)
  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}-qt4")
ENDIF(NOT SNAP_USE_QT4)

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

# Configure version-specific Qt code
IF(SNAP_USE_QT4)
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/GUI/Qt/Components/SNAPOpenGLWidget_Qt4.h.in
    ${SNAP_BINARY_DIR}/SNAPOpenGLWidget.h @ONLY IMMEDIATE)
ELSE()
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/GUI/Qt/Components/SNAPOpenGLWidget_Qt5.h.in
    ${SNAP_BINARY_DIR}/SNAPOpenGLWidget.h @ONLY IMMEDIATE)
ENDIF()

# The part of the source code devoted to the SNAP application logic
# is organized into a separate library
SET(LOGIC_CXX
  ${SNAP_BINARY_DIR}/SNAPCommon.cxx
  Common/AbstractModel.cxx
  Common/AbstractPropertyContainerModel.cxx
  Common/AffineTransformHelper.cxx
  Common/ColorLabelPropertyModel.cxx
  Common/CommandLineArgumentParser.cxx
  Common/EventBucket.cxx
  Common/ExtendedGDCMSerieHelper.cxx
  Common/HistoryManager.cxx
  Common/IPCHandler.cxx
  Common/IRISException.cxx
  Common/Rebroadcaster.cxx
  Common/Registry.cxx
  Common/SNAPEvents.cxx
  Common/SNAPOpenGL.cxx
  Common/SystemInterface.cxx
  Common/TagList.cxx
  Common/ThreadSpecificData.cxx
  Common/Trackball.cxx
  Common/ITKExtras/itkVoxBoCUBImageIO.cxx
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.cxx
  Common/JSon/jsoncpp.cpp
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
  Logic/ImageWrapper/LabelImageWrapper.cxx
  Logic/ImageWrapper/GuidedNativeImageIO.cxx
  Logic/ImageWrapper/MultiChannelDisplayMode.cxx
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
  Logic/Preprocessing/RFClassificationEngine.cxx
  Logic/Preprocessing/Texture/MomentTextures.cxx
  Logic/Slicing/IntensityCurveVTK.cxx
  Logic/Slicing/IntensityToColorLookupTableImageFilter.cxx
  Logic/Slicing/LookupTableIntensityMappingFilter.cxx
  Logic/Slicing/RGBALookupTableIntensityMappingFilter.cxx
  Logic/WorkspaceAPI/CSVParser.cxx
  Logic/WorkspaceAPI/FormattedTable.cxx
  Logic/WorkspaceAPI/RESTClient.cxx
  Logic/WorkspaceAPI/WorkspaceAPI.cxx
)

# The headers for the Logic code
SET(LOGIC_HEADERS
  ${SNAP_BINARY_DIR}/GPUSettings.h
  Common/AbstractModel.h
  Common/AbstractPropertyContainerModel.h
  Common/AffineTransformHelper.h
  Common/ColorLabelPropertyModel.h
  Common/CommandLineArgumentParser.h
  Common/Credits.h
  Common/ExtendedGDCMSerieHelper.h
  Common/HistoryManager.h
  Common/ImageFunctions.h
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
  Common/ITKExtras/itkMorphologicalContourInterpolator.h
  Common/ITKExtras/itkMorphologicalContourInterpolator.hxx
  Common/ITKExtras/itkParallelSparseFieldLevelSetImageFilterBugFix.h
  Common/ITKExtras/itkParallelSparseFieldLevelSetImageFilterBugFix.txx
  Common/ITKExtras/itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.h
  Common/ITKExtras/itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.txx
  Common/ITKExtras/itkVoxBoCUBImageIO.h
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.h
  Common/JSon/json/json.h
  Common/JSon/json/json-forwards.h
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
  Common/TagList.h
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
  Logic/Framework/SegmentationUpdateIterator.h
  Logic/Framework/SNAPImageData.h
  Logic/Framework/UndoDataManager.h
  Logic/Framework/UndoDataManager.txx
  Logic/ImageWrapper/CommonRepresentationPolicy.h
  Logic/ImageWrapper/DisplayMappingPolicy.h
  Logic/ImageWrapper/GuidedNativeImageIO.h
  Logic/ImageWrapper/ImageWrapper.h
  Logic/ImageWrapper/ImageWrapperBase.h
  Logic/ImageWrapper/ImageWrapperTraits.h
  Logic/ImageWrapper/MultiChannelDisplayMode.h
  Logic/ImageWrapper/VectorToScalarImageAccessor.h
  Logic/RLEImage/RLEImage.h
  Logic/RLEImage/RLEImage.txx
  Logic/RLEImage/RLEImageConstIterator.h
  Logic/RLEImage/RLEImageIterator.h
  Logic/RLEImage/RLEImageRegionConstIterator.h
  Logic/RLEImage/RLEImageRegionIterator.h
  Logic/RLEImage/RLEImageScanlineConstIterator.h
  Logic/RLEImage/RLEImageScanlineIterator.h
  Logic/RLEImage/RLERegionOfInterestImageFilter.h
  Logic/RLEImage/RLERegionOfInterestImageFilter.txx
  Logic/ImageWrapper/InputSelectionImageFilter.h
  Logic/ImageWrapper/LabelImageWrapper.h
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
  Logic/Preprocessing/Texture/MomentTextures.h
  Logic/Slicing/ImageRegionConstIteratorWithIndexOverride.h
  Logic/Slicing/FastLinearInterpolator.h
  Logic/Slicing/IRISSlicer.h
  Logic/Slicing/IRISSlicer.txx
  Logic/Slicing/IRISSlicer_RLE.txx
  Logic/Slicing/IntensityCurveInterface.h
  Logic/Slicing/IntensityCurveVTK.h
  Logic/Slicing/IntensityToColorLookupTableImageFilter.h
  Logic/Slicing/LookupTableIntensityMappingFilter.h
  Logic/Slicing/NonOrthogonalSlicer.h
  Logic/Slicing/NonOrthogonalSlicer.txx
  Logic/Slicing/RGBALookupTableIntensityMappingFilter.h
  Logic/WorkspaceAPI/CSVParser.h
  Logic/WorkspaceAPI/FormattedTable.h
  Logic/WorkspaceAPI/RESTClient.h
  Logic/WorkspaceAPI/WorkspaceAPI.h
)

# These files have the UI model code, which is GUI-TK independent
SET(UI_GENERIC_CXX
  GUI/Model/AnnotationModel.cxx
  GUI/Model/ColorLabelQuickListModel.cxx
  GUI/Model/ColorMapModel.cxx
  GUI/Model/CursorInspectionModel.cxx
  GUI/Model/DisplayLayoutModel.cxx
  GUI/Model/DistributedSegmentationModel.cxx
  GUI/Model/Generic3DModel.cxx
  GUI/Model/GenericSliceModel.cxx
  GUI/Model/GlobalPreferencesModel.cxx
  GUI/Model/GlobalUIModel.cxx
  GUI/Model/ImageIOWizardModel.cxx
  GUI/Model/ImageInfoModel.cxx
  GUI/Model/IntensityCurveModel.cxx
  GUI/Model/InteractiveRegistrationModel.cxx
  GUI/Model/InterpolateLabelModel.cxx
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
  GUI/Model/RegistrationModel.cxx
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
  GUI/Model/UIReporterDelegates.cxx
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
  GUI/Renderer/IntensityUnderCursorRenderer.cxx
  GUI/Renderer/LayerHistogramPlotAssembly.cxx
  GUI/Renderer/OpenGLSliceTexture.cxx
  GUI/Renderer/OptimizationProgressRenderer.cxx
  GUI/Renderer/OrientationGraphicRenderer.cxx
  GUI/Renderer/PaintbrushRenderer.cxx
  GUI/Renderer/PolygonDrawingRenderer.cxx
  GUI/Renderer/PolygonScanConvert.cxx
  GUI/Renderer/RegistrationRenderer.cxx
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
  GUI/Model/CursorInspectionModel.h
  GUI/Model/DisplayLayoutModel.h
  GUI/Model/DistributedSegmentationModel.h
  GUI/Model/Generic3DModel.h
  GUI/Model/GenericSliceModel.h
  GUI/Model/GlobalPreferencesModel.h
  GUI/Model/GlobalUIModel.h
  GUI/Model/ImageInfoModel.h
  GUI/Model/ImageIOWizardModel.h
  GUI/Model/IntensityCurveModel.h
  GUI/Model/InteractiveRegistrationModel.h
  GUI/Model/InterpolateLabelModel.h
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
  GUI/Model/RegistrationModel.h
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
  GUI/Renderer/IntensityUnderCursorRenderer.h
  GUI/Renderer/LayerHistogramPlotAssembly.h
  GUI/Renderer/OptimizationProgressRenderer.h
  GUI/Renderer/OrientationGraphicRenderer.h
  GUI/Renderer/PaintbrushRenderer.h
  GUI/Renderer/PolygonDrawingRenderer.h
  GUI/Renderer/PolygonScanConvert.h
  GUI/Renderer/RegistrationRenderer.h
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
  GUI/Qt/Components/DICOMListingTable.cxx
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
  GUI/Qt/Components/QtFlowLayout.cxx
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
  GUI/Qt/Components/SNAPQApplication.cxx
  GUI/Qt/Components/SNAPQtCommon.cxx
  GUI/Qt/Components/SliceViewPanel.cxx
  GUI/Qt/Components/SynchronizationInspector.cxx
  GUI/Qt/Components/TagListWidget.cxx
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
  GUI/Qt/View/RegistrationInteractionMode.cxx
  GUI/Qt/View/SliceWindowInteractionDelegateWidget.cxx
  GUI/Qt/View/SnakeROIInteractionMode.cxx
  GUI/Qt/View/ThumbnailInteractionMode.cxx
  GUI/Qt/Windows/AboutDialog.cxx
  GUI/Qt/Windows/AnnotationEditDialog.cxx
  GUI/Qt/Windows/DropActionDialog.cxx
  GUI/Qt/Windows/ImageIODialog.cxx
  GUI/Qt/Windows/ImageIOWizard.cxx
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.cxx
  GUI/Qt/Windows/InterpolateLabelsDialog.cxx
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
  GUI/Qt/Windows/DSS/DistributedSegmentationDialog.cxx
  GUI/Qt/Windows/DSS/DownloadTicketDialog.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.cxx
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.cxx
  GUI/Qt/Windows/Registration/RegistrationDialog.cxx
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
  GUI/Qt/Components/DICOMListingTable.h
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
  GUI/Qt/Components/QtFlowLayout.h
  GUI/Qt/Components/QtHideOnDeactivateContainer.h
  GUI/Qt/Components/QtIPCManager.h
  GUI/Qt/Components/QtWarningDialog.h
  GUI/Qt/Components/QtWidgetActivator.h
  GUI/Qt/Components/RecentHistoryItemsView.h
  GUI/Qt/Components/SnakeToolROIPanel.h
  GUI/Qt/Components/SnakeWizardPanel.h
  GUI/Qt/Components/SNAPComponent.h
  GUI/Qt/Components/SNAPQApplication.h
  GUI/Qt/Components/SliceViewPanel.h
  GUI/Qt/Components/SynchronizationInspector.h
  GUI/Qt/Components/TagListWidget.h
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
  GUI/Qt/View/RegistrationInteractionMode.h
  GUI/Qt/View/SliceWindowInteractionDelegateWidget.h
  GUI/Qt/View/SnakeROIInteractionMode.h
  GUI/Qt/View/TestOpenGLDialog.h
  GUI/Qt/View/ThumbnailInteractionMode.h
  GUI/Qt/Windows/AboutDialog.h
  GUI/Qt/Windows/AnnotationEditDialog.h
  GUI/Qt/Windows/DropActionDialog.h
  GUI/Qt/Windows/ImageIODialog.h
  GUI/Qt/Windows/ImageIOWizard.h
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.h
  GUI/Qt/Windows/InterpolateLabelsDialog.h
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
  GUI/Qt/Windows/DSS/DistributedSegmentationDialog.h
  GUI/Qt/Windows/DSS/DownloadTicketDialog.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.h
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.h
  GUI/Qt/Windows/Registration/RegistrationDialog.h
  Testing/GUI/Qt/SNAPTestQt.h
)

# These UI headers don't need to be MOC'd
SET(UI_NONMOC_HEADERS
  GUI/Qt/Components/ProcessEventsITKCommand.h
  GUI/Qt/Components/QtCursorOverride.h
  GUI/Qt/Components/QtRendererPlatformSupport.h
  GUI/Qt/Components/QtReporterDelegates.h
  GUI/Qt/Components/SNAPQtCommon.h
  ${SNAP_BINARY_DIR}/SNAPOpenGLWidget.h
  GUI/Qt/Coupling/TagListWidgetCoupling.h
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
  GUI/Qt/Coupling/QtProgressBarCoupling.h
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
  GUI/Qt/Windows/AnnotationEditDialog.ui
  GUI/Qt/Windows/DropActionDialog.ui
  GUI/Qt/Windows/ImageIODialog.ui
  GUI/Qt/Windows/ImageIOWizard/OverlayRolePage.ui
  GUI/Qt/Windows/InterpolateLabelsDialog.ui
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
  GUI/Qt/Windows/DSS/DistributedSegmentationDialog.ui
  GUI/Qt/Windows/DSS/DownloadTicketDialog.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportWizard.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportModePage.ui
  GUI/Qt/Windows/MeshExportWizard/MeshExportBrowsePage.ui
  GUI/Qt/Windows/Registration/RegistrationDialog.ui
)

SET(UI_RESOURCES
  GUI/Qt/Resources/SNAPResources.qrc
  GUI/Qt/Resources/SNAPResources_Linux.qrc
  GUI/Qt/Resources/SNAPResources_MacOS.qrc
  GUI/Qt/Resources/SNAPResources_Windows.qrc
  Testing/GUI/Qt/TestingScripts.qrc
)

# The source code for ITK-SNAP testing project
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

# These are the syetem include directories
SET(SNAP_SYSTEM_INCLUDE_DIRS
  ${ITK_DIR}/Utilities/zlib
  ${SNAP_BINARY_DIR}
  ${OPENGL_INCLUDE_PATH}
  ${SNAP_QT_INCLUDE_DIRS})

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
  SET(SNAP_INCLUDE_DIRS 
    ${CONFIGURED_SOURCE_DIRECTORY}
    ${SNAP_SYSTEM_INCLUDE_DIRS})

ELSE( CMAKE_GENERATOR MATCHES "Visual Studio 6" )

  # Include directories
  SET(SNAP_INCLUDE_DIRS
    ${SNAP_SOURCE_DIR}/Common
    ${SNAP_SOURCE_DIR}/Common/ITKExtras
    ${SNAP_SOURCE_DIR}/Common/JSon
    ${SNAP_SOURCE_DIR}/Logic
    ${SNAP_SOURCE_DIR}/Logic/Common
    ${SNAP_SOURCE_DIR}/Logic/Framework
    ${SNAP_SOURCE_DIR}/Logic/ImageWrapper
    ${SNAP_SOURCE_DIR}/Logic/LevelSet
    ${SNAP_SOURCE_DIR}/Logic/Mesh
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing/GMM
    ${SNAP_SOURCE_DIR}/Logic/Preprocessing/Texture
    ${SNAP_SOURCE_DIR}/Logic/RLEImage
    ${SNAP_SOURCE_DIR}/Logic/Slicing
    ${SNAP_SOURCE_DIR}/Logic/WorkspaceAPI
    ${SNAP_SOURCE_DIR}/Submodules/c3d
    ${SNAP_SOURCE_DIR}/Submodules/c3d/itkextras
    ${SNAP_SOURCE_DIR}/Submodules/c3d/itkextras/RandomForest
    ${SNAP_SOURCE_DIR}/Submodules/c3d/itkextras/PovRayIO
    ${SNAP_SOURCE_DIR}/Submodules/c3d/api
    ${SNAP_SOURCE_DIR}/Submodules/greedy/src
    ${SNAP_SOURCE_DIR}/Submodules/greedy/src/ITKFilters/include
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
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows/DSS
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows/MeshExportWizard
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows/Registration
    ${SNAP_SOURCE_DIR}/Testing/Logic
    ${SNAP_SOURCE_DIR}/Testing/GUI/Qt
    ${SNAP_SYSTEM_INCLUDE_DIRS}
  )

ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )

#--------------------------------------------------------------------------------
# Compiler-specific warinings
#--------------------------------------------------------------------------------

# Get rid of this ridiculous warning in VS8+
IF( MSVC )
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
  ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF( MSVC )
  
IF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )
  ADD_DEFINITIONS(-DNOMINMAX)
ENDIF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )

#--------------------------------------------------------------------------------
# Define External Libraries
#--------------------------------------------------------------------------------

# ITK Libraries
SET(SNAP_ITK_LIBS ${ITK_LIBRARIES})

# Core VTK libraries
SET(SNAP_VTK_LIBS ${VTK_LIBRARIES})

# Libraries provided by the submodules
SET(SNAP_SUBMODULE_LIBS
  cnd_api
  cnd_driver
  cnd_adapters
  greedyapi)

# System libraries
SET(SNAP_SYSTEM_LIBS
  ${SNAP_OPENGL_LIBS}
  ${CURL_LIBRARIES}
  ${SYSTEM_LIBS}
)

# Designate the external libraries used by SNAP
SET(SNAP_EXTERNAL_LIBS 
  ${SNAP_SUBMODULE_LIBS}
  ${SNAP_ITK_LIBS} 
  ${SNAP_VTK_LIBS} 
  ${SNAP_SYSTEM_LIBS}
)

# Non-graphical external libs (a subset)
SET(SNAP_EXTERNAL_LIBS_LITE
  ${SNAP_SUBMODULE_LIBS}
  ${SNAP_ITK_LIBS} 
  vtkCommonCore vtkFiltersCore
  ${CURL_LIBRARIES}
  ${SYSTEM_LIBS})


#--------------------------------------------------------------------------------
# Include C3D Library as submodule
#--------------------------------------------------------------------------------
SET(CONVERT3D_SOURCE_DIR ${SNAP_SOURCE_DIR}/Submodules/c3d)
SET(CONVERT3D_BINARY_DIR ${SNAP_BINARY_DIR}/Submodules/c3d)
SET(CONVERT3D_BUILD_AS_SUBPROJECT ON)
SET(CONVERT3D_SUBPROJECT_BUILD_CLI_TOOLS ON)
SET(CONVERT3D_SUBPROJECT_INSTALL_CLI_TOOLS ON)
SET(CONVERT3D_SUBPROJECT_CLI_INSTALL_PATH ${SNAP_CLI_INSTALL_PATH})
ADD_SUBDIRECTORY(${CONVERT3D_SOURCE_DIR})

#--------------------------------------------------------------------------------
# Include Greedy Library as submodule
#--------------------------------------------------------------------------------
SET(GREEDY_SOURCE_DIR ${SNAP_SOURCE_DIR}/Submodules/greedy)
SET(GREEDY_BINARY_DIR ${SNAP_BINARY_DIR}/Submodules/greedy)
SET(GREEDY_BUILD_AS_SUBPROJECT ON)
SET(GREEDY_SUBPROJECT_BUILD_CLI ON)
SET(GREEDY_SUBPROJECT_INSTALL_CLI ON)
SET(GREEDY_SUBPROJECT_CLI_INSTALL_PATH ${SNAP_CLI_INSTALL_PATH})
ADD_SUBDIRECTORY(${GREEDY_SOURCE_DIR})


#--------------------------------------------------------------------------------
# Define SNAP library targets (logic and UI)
#--------------------------------------------------------------------------------

# Wrap the QT input files
IF(NOT SNAP_USE_QT4)
  QT5_WRAP_UI(UI_FORM_HEADERS ${UI_FORMS})
  QT5_WRAP_CPP(UI_WRAPPED_MOC_HEADERS ${UI_MOC_HEADERS})
  QT5_ADD_RESOURCES(UI_RESOURCES_RCC ${UI_RESOURCES})
ELSE(NOT SNAP_USE_QT4)
  QT4_WRAP_UI(UI_FORM_HEADERS ${UI_FORMS})
  QT4_WRAP_CPP(UI_WRAPPED_MOC_HEADERS ${UI_MOC_HEADERS})
  QT4_ADD_RESOURCES(UI_RESOURCES_RCC ${UI_RESOURCES})
ENDIF(NOT SNAP_USE_QT4)

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
TARGET_LINK_LIBRARIES(itksnaplogic ${SNAP_EXTERNAL_LIBS_LITE})
TARGET_LINK_LIBRARIES(itksnapui_model itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui_qt ${SNAP_QT_LIBRARIES} ${SNAP_EXTERNAL_LIBS})

# Designate the SNAP internal libraries
SET(SNAP_INTERNAL_LIBS itksnapui_qt itksnapui_model itksnaplogic)

# Set up include directories for these targets
FOREACH(target ${SNAP_INTERNAL_LIBS})
  TARGET_INCLUDE_DIRECTORIES(${target} PUBLIC ${SNAP_INCLUDE_DIRS})
ENDFOREACH()

#--------------------------------------------------------------------------------
# Define main SNAP executable
#--------------------------------------------------------------------------------

# Code for the main application
SET(SNAP_MAIN_SRC GUI/Qt/main.cxx)

# On Apple, configure the application icon
IF(APPLE)

  # the icon file
  SET(SNAP_OSX_ICON ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap.icns)

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
ADD_EXECUTABLE(${SNAP_BUNDLE_NAME} WIN32 ${SNAP_MAIN_SRC})
TARGET_LINK_LIBRARIES(${SNAP_BUNDLE_NAME} ${SNAP_INTERNAL_LIBS} ${SNAP_EXTERNAL_LIBS})
TARGET_INCLUDE_DIRECTORIES(${SNAP_BUNDLE_NAME} PUBLIC ${SNAP_INCLUDE_DIRS})

#--------------------------------------------------------------------------------
# Testing-related executables
#--------------------------------------------------------------------------------

ENABLE_TESTING()

include(BundleUtilities)

# Acceleration factor for GUI tests
SET(SNAP_GUI_TEST_ACCEL "1.0" CACHE STRING 
    "Acceleration factor for GUI tests, use <1 on VMS and slow machines")
MARK_AS_ADVANCED(SNAP_GUI_TEST_ACCEL)

# The list of Qt animated GUI tests
SET(GUI_TESTS
  Workspace
  DiffSpace
  ProbeIntensity
  RegionCompetition
  RandomForest
  RandomForestBailOut)

SET(TESTDATA_DIR "${SNAP_SOURCE_DIR}/Testing/TestData")
set(TEMP ${SNAP_BINARY_DIR}/Testing/Temporary)

ADD_EXECUTABLE(SlicingPerformanceTest Testing/Logic/SlicingPerformanceTest.cxx)
TARGET_LINK_LIBRARIES(SlicingPerformanceTest ${ITK_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(SlicingPerformanceTest PUBLIC ${SNAP_INCLUDE_DIRS})

ADD_EXECUTABLE(testRLE Testing/Logic/testRLE.cxx)
TARGET_LINK_LIBRARIES(testRLE ${ITK_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(testRLE PUBLIC ${SNAP_INCLUDE_DIRS})

ADD_EXECUTABLE(iteratorTests
    Testing/Logic/itkRegionOfInterestImageFilterTest.cxx
    Testing/Logic/itkIteratorTests.cxx
    Testing/Logic/itkImageIteratorsForwardBackwardTest.cxx
    Testing/Logic/itkImageIteratorTest.cxx
    Testing/Logic/itkImageIteratorWithIndexTest.cxx
    Testing/Logic/itkImageRegionConstIteratorWithOnlyIndexTest.cxx
    Testing/Logic/itkImageRegionIteratorTest.cxx
    Testing/Logic/itkImageScanlineIteratorTest1.cxx
    Testing/Logic/iteratorTests.cxx)
TARGET_LINK_LIBRARIES(iteratorTests ${ITK_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(iteratorTests PUBLIC ${SNAP_INCLUDE_DIRS})

add_test(NAME BasicSlicingTestX39 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/X39.nii.gz ${TEMP}/X39.nii.gz
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/MRIcrop-seg.gipl.gz
        ${TEMP}/X39.nii.gz
        X 39 irisRLE
)

add_test(NAME BasicSlicingTestY55 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/Y55.nii.gz ${TEMP}/Y55.nii.gz
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/MRIcrop-seg.gipl.gz
        ${TEMP}/Y55.nii.gz
        Y 55 irisRLE
)

add_test(NAME BasicSlicingTestZ32 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/Z32.nii.gz ${TEMP}/Z32.nii.gz
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/MRIcrop-seg.gipl.gz
        ${TEMP}/Z32.nii.gz
        Z 32 irisRLE
)

add_test(NAME SlicingPerformanceTestX300 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/X300.mha ${TEMP}/X300.mha
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/vb-seg.mha
        ${TEMP}/X300.mha
        X 300 irisRLE
)

add_test(NAME SlicingPerformanceTestY300 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/Y300.mha ${TEMP}/Y300.mha
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/vb-seg.mha
        ${TEMP}/Y300.mha
        Y 300 irisRLE
)

add_test(NAME SlicingPerformanceTestZ150 COMMAND itkTestDriver
  --compare ${TESTDATA_DIR}/Z150.mha ${TEMP}/Z150.mha
  $<TARGET_FILE:SlicingPerformanceTest>
        ${TESTDATA_DIR}/vb-seg.mha
        ${TEMP}/Z150.mha
        Z 150 irisRLE
)

# This test basically checks whether we can build using the logic library onlu
ADD_EXECUTABLE(logic_api_test
    Testing/Logic/IRISApplicationTest.cxx)
TARGET_LINK_LIBRARIES(logic_api_test ${SNAP_EXTERNAL_LIBS} itksnaplogic)
TARGET_INCLUDE_DIRECTORIES(logic_api_test PUBLIC ${SNAP_INCLUDE_DIRS})

add_test(NAME IRISApplicationTest COMMAND logic_api_test)

# Set up a test for each GUI test
FOREACH(GUI_TEST ${GUI_TESTS})

  # The path of the test - not obvious?
  IF(APPLE)
    GET_PROPERTY(GUI_TEST_EXE TARGET ${SNAP_BUNDLE_NAME} PROPERTY LOCATION)
  ELSE(APPLE)
    SET(GUI_TEST_EXE ${SNAP_BUNDLE_NAME})
  ENDIF(APPLE)
    
  # Add the test
  ADD_TEST(${GUI_TEST} ${GUI_TEST_EXE} --test ${GUI_TEST} --testacc ${SNAP_GUI_TEST_ACCEL} --testdir "${TESTDATA_DIR}")

  # Set the environment for the test to include Qt
  FILE(TO_NATIVE_PATH ${QT_BINARY_DIR} QT_BINARY_DIR_NATIVE)

  IF(WIN32)
    SET_PROPERTY(TEST ${GUI_TEST} APPEND PROPERTY ENVIRONMENT PATH=${QT_BINARY_DIR_NATIVE})
  ELSE(WIN32)
    SET_PROPERTY(TEST ${GUI_TEST} APPEND PROPERTY ENVIRONMENT PATH="${QT_BINARY_DIR_NATIVE}:$ENV{PATH}")
  ENDIF(WIN32)
  SET_PROPERTY(TEST ${GUI_TEST} PROPERTY TIMEOUT 180)

ENDFOREACH(GUI_TEST)

# Orientation widget test
SET(TEST_ORIENTATION_WIDGET_HEADERS
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h
)

SET(TEST_ORIENTATION_WIDGET_CXX
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.cxx
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/main.cxx
)

IF(NOT SNAP_USE_QT4)
  QT5_WRAP_UI(UI_ORIENTATION_WIDGET_SRCS 
    GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.ui)
  QT5_WRAP_CPP(MOC_ORIENTATION_WIDGET_SRCS 
    GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h)
ELSE(NOT SNAP_USE_QT4)
  QT4_WRAP_UI(UI_ORIENTATION_WIDGET_SRCS 
    GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.ui)
  QT4_WRAP_CPP(MOC_ORIENTATION_WIDGET_SRCS 
    GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h)
ENDIF(NOT SNAP_USE_QT4)

ADD_EXECUTABLE(Test_OrientationWidget
  ${TEST_ORIENTATION_WIDGET_CXX} ${TEST_ORIENTATION_WIDGET_HEADERS}
  ${UI_ORIENTATION_WIDGET_SRCS} ${MOC_ORIENTATION_WIDGET_SRCS}
)

TARGET_LINK_LIBRARIES(Test_OrientationWidget
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS}
)

TARGET_INCLUDE_DIRECTORIES(Test_OrientationWidget PUBLIC ${SNAP_INCLUDE_DIRS})

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

#--------------------------------------------------------------------------------
# Install the application

IF(APPLE)

  # Use the Bundle insaller
  SET(CPACK_GENERATOR Bundle)
  SET(CPACK_EXTENSION "dmg")

  # Set the name of the application
  SET(CPACK_BUNDLE_NAME "ITK-SNAP")

  # Set the info.plist file
  SET(CPACK_BUNDLE_PLIST_SOURCE ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/Info.plist)
  SET(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
  CONFIGURE_FILE("${CPACK_BUNDLE_PLIST_SOURCE}" "${CPACK_BUNDLE_PLIST}" @ONLY)
  INSTALL(FILES ${CPACK_BUNDLE_PLIST} DESTINATION ..)

  # The icon
  SET(CPACK_BUNDLE_ICON ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap.icns)
  SET(CPACK_PACKAGE_ICON ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/itksnap.icns)

  # Code signing certificate - optional
  SET(SNAP_MACOSX_CODESIGN_CERT "" CACHE STRING "Name of the Apple Developer Certificate to sign application")
  MARK_AS_ADVANCED(SNAP_MACOSX_CODESIGN_CERT)
  IF(SNAP_MACOSX_CODESIGN_CERT)
    SET(CPACK_BUNDLE_APPLE_CERT_APP ${SNAP_MACOSX_CODESIGN_CERT})
  ENDIF()

  # Install the main executable
  SNAP_INSTALL_GUI(${SNAP_BUNDLE_NAME} itksnap "--console")

  # Set the DS STORE
  SET(CPACK_DMG_DS_STORE_SETUP_SCRIPT ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/make_dsstore.scpt)
  SET(CPACK_DMG_BACKGROUND_IMAGE ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/background.tiff)

  # Install the command-line installer
  INSTALL(FILES
    ${SNAP_SOURCE_DIR}/Utilities/MacOS/BundleResources/install_cmdl.sh
    DESTINATION ../bin COMPONENT Runtime
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    GROUP_EXECUTE GROUP_READ)

  IF(NOT SNAP_USE_QT4)

    # Include the qt4 dependent libraries
    include(${SNAP_SOURCE_DIR}/CMake/DeployQt5.cmake)

    # Make sure the GIF plugin is included
    get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

    get_property(loc TARGET Qt5::QCocoaIntegrationPlugin PROPERTY LOCATION_RELEASE)

    # Install with the plugin
    install_qt5_executable(../../../${SNAP_BUNDLE_NAME}.app "${QT_GIF_PLUGIN}")

  ELSE(NOT SNAP_USE_QT4)

    # Include the qt4 dependent libraries
    include(DeployQt4)
    install_qt4_executable(${SNAP_BUNDLE_NAME}.app "qsqlite;qgif")

  ENDIF(NOT SNAP_USE_QT4)

ELSEIF(WIN32)

  # Install to the bin directory
  INSTALL(TARGETS ${SNAP_BUNDLE_NAME} RUNTIME DESTINATION bin)

  IF(NOT SNAP_USE_QT4)

      # Include the qt4 dependent libraries
      include(${SNAP_SOURCE_DIR}/CMake/DeployQt5.cmake)

      # Make sure the GIF plugin is included
      get_property(QT_WIN_PLUGIN TARGET Qt5::QWindowsIntegrationPlugin PROPERTY LOCATION_RELEASE)
      get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

      # Install with the plugin
      install_qt5_executable(bin/${SNAP_BUNDLE_NAME}.exe "${QT_GIF_PLUGIN};${QT_WIN_PLUGIN}"  "" "${VTK_RUNTIME_LIBRARY_DIRS}")

  ELSE(NOT SNAP_USE_QT4)

      # Include the qt4 dependent libraries
      include(DeployQt4)
      install_qt4_executable(bin/${SNAP_BUNDLE_NAME}.exe "qsqlite;qgif")

  ENDIF(NOT SNAP_USE_QT4)

  # On windows, we have to configure NSIS
  SET(CPACK_NSIS_MUI_ICON "${SNAP_SOURCE_DIR}/Utilities/Win32/snaplogo.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "ITK-SNAP.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ITK-SNAP")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.itksnap.org")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.itksnap.org/credits.php")
  SET(CPACK_NSIS_MODIFY_PATH ON)
  SET(CPACK_NSIS_MENU_LINKS "http://itksnap.org" "ITK-SNAP Web Site")

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

  IF(NOT SNAP_USE_QT4)

    # Include the qt4 dependent libraries
    include(${SNAP_SOURCE_DIR}/CMake/DeployQt5.cmake)

    # Add an option to not package QT plugins. This is needed for some special builds,
    # for example on Debian
    OPTION(SNAP_PACKAGE_QT_PLUGINS "Try to package Qt5 plugins with the executable" ON)
    MARK_AS_ADVANCED(SNAP_PACKAGE_QT_PLUGINS)

    # Get the necessary plugins
    IF(SNAP_PACKAGE_QT_PLUGINS)

      get_property(QT_XCB_PLUGIN TARGET Qt5::QXcbIntegrationPlugin PROPERTY LOCATION_RELEASE)
      get_property(QT_XCB_GLX_PLUGIN TARGET Qt5::QXcbEglIntegrationPlugin PROPERTY LOCATION_RELEASE)
      get_property(QT_XCB_EGL_PLUGIN TARGET Qt5::QXcbGlxIntegrationPlugin PROPERTY LOCATION_RELEASE)
      get_property(QT_GIF_PLUGIN TARGET Qt5::QGifPlugin PROPERTY LOCATION_RELEASE)

      # Install the plugin
      install_qt5_executable(${SNAP_MAIN_INSTALL_DIR}/${SNAP_EXE} 
        "${QT_XCB_PLUGIN};${QT_GIF_PLUGIN};${QT_XCB_GLX_PLUGIN};${QT_XCB_EGL_PLUGIN}")

    ELSE(SNAP_PACKAGE_QT_PLUGINS)

      # Install the exe without plugins
      install_qt5_executable(${SNAP_MAIN_INSTALL_DIR}/${SNAP_EXE})

    ENDIF(SNAP_PACKAGE_QT_PLUGINS)

  ELSE(NOT SNAP_USE_QT4)

    include(DeployQt4)
    install_qt4_executable(${SNAP_MAIN_INSTALL_DIR}/${SNAP_EXE} "qgif")

  ENDIF(NOT SNAP_USE_QT4)

ENDIF()


#--------------------------------------------------------------------------------
# Configure CPack

# Which generator to use
IF(APPLE)
  # Above
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

IF(NOT SNAP_USE_QT4)
  SET(NIGHTLY_TARGET "itksnap-nightly-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")
  SET(EXPERIMENTAL_TARGET "itksnap-experimental-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")
ELSE(NOT SNAP_USE_QT4)
  SET(NIGHTLY_TARGET "itksnap-nightly-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}-qt4.${CPACK_EXTENSION}")
  SET(EXPERIMENTAL_TARGET "itksnap-experimental-${SNAP_VERSION_GIT_BRANCH}-${CPACK_SYSTEM_NAME}-qt4.${CPACK_EXTENSION}")
ENDIF(NOT SNAP_USE_QT4)

SET(SCP_ROOT "frs.sourceforge.net:/home/frs/project/i/it/itk-snap/itk-snap")

#--------------------------------------------------------------------------------
# Create targets

ADD_CUSTOM_TARGET(itksnap_upload_nightly
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS}
  ${CPACK_PACKAGE_FILE_NAME_WEXT} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Nightly/${NIGHTLY_TARGET}
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_PACKAGE_FILE_NAME_WEXT} to SourceForge.net as ${NIGHTLY_TARGET}")

ADD_CUSTOM_TARGET(itksnap_upload_experimental
  VERBATIM COMMAND "${SCP_PROGRAM}" ${SCP_ARGUMENTS} 
    ${CPACK_PACKAGE_FILE_NAME_WEXT} ${SCP_USERNAME},itk-snap@${SCP_ROOT}/Experimental/${EXPERIMENTAL_TARGET}
  DEPENDS ${CPACK_TARGET}
  WORKING_DIRECTORY ${SNAP_BINARY_DIR}
  COMMENT "Uploading package ${CPACK_PACKAGE_FILE_NAME_WEXT} to SourceForge.net as ${EXPERIMENTAL_TARGET}")


#--------------------------------------------------------------------------------
# ALFABIS utilities (experimental)
#--------------------------------------------------------------------------------
# Workspace tool is automatically included
ADD_SUBDIRECTORY(${SNAP_SOURCE_DIR}/Utilities/Workspace)


#--------------------------------------------------------------------------------
# OPTIONAL INCLUDE-WHAT-YOU-USE code checker
#--------------------------------------------------------------------------------
OPTION(SNAP_USE_IWYU "Perform code profiling using include-what-you-use tool" OFF)
MARK_AS_ADVANCED(SNAP_USE_IWYU)

IF(SNAP_USE_IWYU)
  FIND_PROGRAM(IWYU_PATH NAMES include-what-you-use iwyu)
  if(NOT IWYU_PATH)
    message(FATAL_ERROR "Could not find the program include-what-you-use")
  endif()

  set_property(TARGET ${SNAP_BUNDLE_NAME} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
ENDIF()
