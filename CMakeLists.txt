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
SET(SNAP_VERSION_MAJOR 3)
SET(SNAP_VERSION_MINOR 0)
SET(SNAP_VERSION_PATCH 0)
SET(SNAP_VERSION_QUALIFIER "-preview")
SET(SNAP_VERSION_FULL "${SNAP_VERSION_MAJOR}.${SNAP_VERSION_MINOR}.${SNAP_VERSION_PATCH}${SNAP_VERSION_QUALIFIER}")
SET(SNAP_VERSION_RELEASE_DATE "20130101")
SET(SNAP_VERSION_LAST_COMPATIBLE_RELEASE_DATE "20090731")
SET(SNAP_VERSION_RELEASE_DATE_FORMATTED "Jan 1, 2013")

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
  STRING(REPLACE ';' "-" ARCH ${CMAKE_OSX_ARCHITECTURES})
  SET(CPACK_SYSTEM_NAME "MacOS-${ARCH}")
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
  Common/EventBucket.cxx
  Common/HistoryManager.cxx
  Common/IRISException.cxx
  Common/Registry.cxx
  Common/SNAPOpenGL.cxx
  Common/SystemInterface.cxx
  Common/ThreadSpecificData.cxx
  Common/ITKExtras/itkVoxBoCUBImageIO.cxx
  Common/ITKExtras/itkVoxBoCUBImageIOFactory.cxx
  Common/Trackball.cxx
  Logic/Common/ColorMap.cxx
  Logic/Common/ColorLabelTable.cxx
  Logic/Common/ImageCoordinateGeometry.cxx
  Logic/Common/ImageCoordinateTransform.cxx
  Logic/Common/MetaDataAccess.cxx
  Logic/Common/SegmentationStatistics.cxx
  Logic/Common/SNAPRegistryIO.cxx
  Logic/Common/SNAPSegmentationROISettings.cxx
  Logic/Framework/GenericImageData.cxx
  Logic/Framework/GlobalState.cxx
  Logic/Framework/IRISApplication.cxx
  Logic/Framework/IRISImageData.cxx
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
  Logic/Mesh/AllPurposeProgressAccumulator.cxx
  Logic/Mesh/GuidedMeshIO.cxx
  Logic/Mesh/IRISMeshPipeline.cxx
  Logic/Mesh/LevelSetMeshPipeline.cxx
  Logic/Mesh/MeshObject.cxx
  Logic/Mesh/MeshOptions.cxx
  Logic/Mesh/VTKMeshPipeline.cxx
  Logic/Preprocessing/EdgePreprocessingSettings.cxx
  Logic/Preprocessing/PreprocessingFilterConfigTraits.cxx
  Logic/Preprocessing/ThresholdSettings.cxx
  Logic/Slicing/IntensityCurveVTK.cxx
  Logic/Slicing/IntensityToColorLookupTableImageFilter.cxx
  Logic/Slicing/LookupTableIntensityMappingFilter.cxx
  Logic/Slicing/RGBALookupTableIntensityMappingFilter.cxx
)

# The headers for the Logic code
SET(LOGIC_HEADERS
  Common/CommandLineArgumentParser.h
  Common/Credits.h
  Common/HistoryManager.h
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
  Common/SNAPEvents.h
  Common/SystemInterface.h
  Common/ThreadSpecificData.h
  Common/Trackball.h
  Logic/Common/ColorLabel.h
  Logic/Common/ColorLabelTable.h
  Logic/Common/ColorMap.h
  Logic/Common/ImageCoordinateGeometry.h
  Logic/Common/ImageCoordinateTransform.h
  Logic/Common/SegmentationStatistics.h
  Logic/Common/ImageRayIntersectionFinder.h
  Logic/Common/ImageRayIntersectionFinder.txx
  Logic/Common/MetaDataAccess.h
  Logic/Common/SNAPRegistryIO.h
  Logic/Common/SNAPSegmentationROISettings.h
  Logic/Framework/GenericImageData.h
  Logic/Framework/GlobalState.h
  Logic/Framework/IRISApplication.h
  Logic/Framework/IRISImageData.h
  Logic/Framework/LayerAssociation.h
  Logic/Framework/LayerAssociation.txx
  Logic/Framework/SNAPImageData.h
  Logic/Framework/UndoDataManager.h
  Logic/Framework/UndoDataManager.txx
  Logic/ImageWrapper/CommonRepresentationPolicy.h
  Logic/ImageWrapper/DisplayMappingPolicy.h
  Logic/ImageWrapper/GuidedNativeImageIO.h
  Logic/ImageWrapper/ImageWrapper.h
  Logic/ImageWrapper/ImageWrapperBase.h
  Logic/ImageWrapper/InputSelectionImageFilter.h
  Logic/ImageWrapper/LabelToRGBAFilter.h
  Logic/ImageWrapper/NativeIntensityMappingPolicy.h
  Logic/ImageWrapper/ScalarImageHistogram.h
  Logic/ImageWrapper/ScalarImageWrapper.h
  Logic/ImageWrapper/VectorImageWrapper.h
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
  Logic/Preprocessing/PreprocessingFilterConfigTraits.h
  Logic/Preprocessing/SlicePreviewFilterWrapper.h
  Logic/Preprocessing/SlicePreviewFilterWrapper.txx
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.h
  Logic/Preprocessing/SmoothBinaryThresholdImageFilter.txx
  Logic/Preprocessing/ThresholdSettings.h
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
  GUI/Model/AbstractLayerAssociatedModel.cxx
  GUI/Model/AbstractModel.cxx
  GUI/Model/ColorLabelPropertyModel.cxx
  GUI/Model/ColorMapModel.cxx
  GUI/Model/ComponentSelectionModel.cxx
  GUI/Model/CursorInspectionModel.cxx
  GUI/Model/DisplayLayoutModel.cxx
  GUI/Model/Generic3DModel.cxx
  GUI/Model/GenericSliceModel.cxx
  GUI/Model/GlobalUIModel.cxx
  GUI/Model/ImageIODelegates.cxx
  GUI/Model/ImageIOWizardModel.cxx
  GUI/Model/ImageInfoModel.cxx
  GUI/Model/IntensityCurveModel.cxx
  GUI/Model/IOActions.cxx
  GUI/Model/LabelEditorModel.cxx
  GUI/Model/LayerSelectionModel.cxx
  GUI/Model/OrthogonalSliceCursorNavigationModel.cxx
  GUI/Model/PaintbrushModel.cxx
  GUI/Model/PaintbrushSettingsModel.cxx
  GUI/Model/PolygonDrawingModel.cxx
  GUI/Model/ReorientImageModel.cxx
  GUI/Model/SliceWindowCoordinator.cxx
  GUI/Model/SNAPAppearanceSettings.cxx
  GUI/Model/SnakeROIModel.cxx
  GUI/Model/SnakeWizardModel.cxx
  GUI/Model/StateManagement.cxx
  GUI/Model/SynchronizationModel.cxx
  GUI/Model/UIAction.cxx
  GUI/Renderer/AbstractRenderer.cxx
  GUI/Renderer/AbstractVTKRenderer.cxx
  GUI/Renderer/AbstractVTKSceneRenderer.cxx
  GUI/Renderer/ColorMapRenderer.cxx
  GUI/Renderer/CrosshairsRenderer.cxx
  GUI/Renderer/EdgePreprocessingSettingsRenderer.cxx
  GUI/Renderer/GenericSliceRenderer.cxx
  GUI/Renderer/Generic3DRenderer.cxx
  GUI/Renderer/GLToPNG.cxx
  GUI/Renderer/IntensityCurveVTKRenderer.cxx
  GUI/Renderer/OpenGLSliceTexture.cxx
  GUI/Renderer/OrientationGraphicRenderer.cxx
  GUI/Renderer/PaintbrushRenderer.cxx
  GUI/Renderer/PolygonDrawingRenderer.cxx
  GUI/Renderer/PolygonScanConvert.cxx
  GUI/Renderer/SliceWindowDecorationRenderer.cxx
  GUI/Renderer/SnakeROIRenderer.cxx
  GUI/Renderer/SnakeModeRenderer.cxx
  GUI/Renderer/ThresholdSettingsRenderer.cxx
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
  GUI/Model/AbstractModel.h
  GUI/Model/ColorMapModel.h
  GUI/Model/ColorLabelPropertyModel.h
  GUI/Model/ComponentSelectionModel.h
  GUI/Model/CursorInspectionModel.h
  GUI/Model/DisplayLayoutModel.h
  GUI/Model/Generic3DModel.h
  GUI/Model/GenericSliceModel.h
  GUI/Model/GlobalUIModel.h
  GUI/Model/ImageInfoModel.h
  GUI/Model/ImageIODelegates.h
  GUI/Model/ImageIOWizardModel.h
  GUI/Model/IntensityCurveModel.h
  GUI/Model/IOActions.h
  GUI/Model/LabelEditorModel.h
  GUI/Model/LayerSelectionModel.h
  GUI/Model/OrthogonalSliceCursorNavigationModel.h
  GUI/Model/PaintbrushModel.h
  GUI/Model/PaintbrushSettingsModel.h
  GUI/Model/PolygonDrawingModel.h
  GUI/Model/PropertyModel.h
  GUI/Model/ReorientImageModel.h
  GUI/Model/SNAPAppearanceSettings.h
  GUI/Model/SNAPUIFlag.h
  GUI/Model/SNAPUIFlag.txx
  GUI/Model/SliceWindowCoordinator.h
  GUI/Model/SnakeROIModel.h
  GUI/Model/SnakeWizardModel.h
  GUI/Model/StateManagement.h
  GUI/Model/SynchronizationModel.h
  GUI/Model/UIAction.h
  GUI/Model/UIReporterDelegates.h
  GUI/Model/UIState.h
  GUI/Renderer/AbstractRenderer.h
  GUI/Renderer/AbstractVTKRenderer.h
  GUI/Renderer/AbstractVTKSceneRenderer.h
  GUI/Renderer/ColorMapRenderer.h
  GUI/Renderer/CrosshairsRenderer.h
  GUI/Renderer/EdgePreprocessingSettingsRenderer.h
  GUI/Renderer/Generic3DRenderer.h
  GUI/Renderer/GenericSliceRenderer.h
  GUI/Renderer/GLToPNG.h
  GUI/Renderer/IntensityCurveVTKRenderer.h
  GUI/Renderer/OrientationGraphicRenderer.h
  GUI/Renderer/PaintbrushRenderer.h
  GUI/Renderer/PolygonDrawingRenderer.h
  GUI/Renderer/PolygonScanConvert.h
  GUI/Renderer/SliceWindowDecorationRenderer.h
  GUI/Renderer/SnakeROIRenderer.h
  GUI/Renderer/SnakeModeRenderer.h
  GUI/Renderer/ThresholdSettingsRenderer.h
  GUI/Renderer/ThresholdSettingsRenderer.cxx
  GUI/Renderer/OrientationWidget/Reorient/AbstractScannerHelper.h
  GUI/Renderer/OrientationWidget/Reorient/AxesWidget.h
  GUI/Renderer/OrientationWidget/Reorient/ScannedHuman.h
  GUI/Renderer/OrientationWidget/Reorient/ScanningROI.h
  GUI/Renderer/OrientationWidget/Reorient/ReorientProps.h
  GUI/Renderer/OrientationWidget/Reorient/PolyDataAlgorithm2ActorPipe.h
)

# These files contain the Qt-specific user interface source code
SET(UI_QT_CXX
  GUI/Qt/Components/ContrastInspector.cxx
  GUI/Qt/Components/ColorMapInspector.cxx
  GUI/Qt/Components/CursorInspector.cxx
  GUI/Qt/Components/DisplayLayoutInspector.cxx
  GUI/Qt/Components/HistoryQListModel.cxx
  GUI/Qt/Components/ImageInfoInspector.cxx
  GUI/Qt/Components/LabelInspector.cxx
  GUI/Qt/Components/LatentITKEventNotifier.cxx
  GUI/Qt/Components/MetadataInspector.cxx
  GUI/Qt/Components/GeneralLayerInspector.cxx
  GUI/Qt/Components/PaintbrushToolPanel.cxx
  GUI/Qt/Components/QActionButton.cxx
  GUI/Qt/Components/QColorButtonWidget.cxx
  GUI/Qt/Components/QDoubleSlider.cxx
  GUI/Qt/Components/QDoubleSliderWithEditor.cxx
  GUI/Qt/Components/QtIPCManager.cxx
  GUI/Qt/Components/QtRendererPlatformSupport.cxx
  GUI/Qt/Components/QtReporterDelegates.cxx
  GUI/Qt/Components/QtWarningDialog.cxx
  GUI/Qt/Components/QtWidgetActivator.cxx
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
  GUI/Qt/Testing/SNAPTestQt.cxx
  GUI/Qt/Testing/QtScriptTest1.cxx
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
  GUI/Qt/Windows/DropActionDialog.cxx
  GUI/Qt/Windows/ImageIODialog.cxx
  GUI/Qt/Windows/ImageIOWizard.cxx
  GUI/Qt/Windows/LabelEditorDialog.cxx
  GUI/Qt/Windows/LayerInspectorDialog.cxx
  GUI/Qt/Windows/MainControlPanel.cxx
  GUI/Qt/Windows/MainImageWindow.cxx
  GUI/Qt/Windows/QtStyles.cxx
  GUI/Qt/Windows/ReorientImageDialog.cxx
  GUI/Qt/Windows/SimpleFileDialogWithHistory.cxx
  GUI/Qt/Windows/SpeedImageDialog.cxx
  GUI/Qt/Windows/SplashPanel.cxx
  GUI/Qt/Windows/StatisticsDialog.cxx
)

# The header files for the UI project
SET(UI_MOC_HEADERS
  GUI/Qt/Components/ContrastInspector.h
  GUI/Qt/Components/ColorMapInspector.h
  GUI/Qt/Components/CursorInspector.h
  GUI/Qt/Components/DisplayLayoutInspector.h
  GUI/Qt/Components/HistoryQListModel.h
  GUI/Qt/Components/ImageInfoInspector.h
  GUI/Qt/Components/LabelInspector.h
  GUI/Qt/Components/LatentITKEventNotifier.h
  GUI/Qt/Components/MetadataInspector.h
  GUI/Qt/Components/GeneralLayerInspector.h
  GUI/Qt/Components/PaintbrushToolPanel.h
  GUI/Qt/Components/QActionButton.h
  GUI/Qt/Components/QColorButtonWidget.h
  GUI/Qt/Components/QDoubleSlider.h
  GUI/Qt/Components/QDoubleSliderWithEditor.h
  GUI/Qt/Components/QtIPCManager.h
  GUI/Qt/Components/QtWarningDialog.h
  GUI/Qt/Components/QtWidgetActivator.h
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
  GUI/Qt/Testing/QtScriptTest1.h
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
  GUI/Qt/Windows/DropActionDialog.h
  GUI/Qt/Windows/ImageIODialog.h
  GUI/Qt/Windows/ImageIOWizard.h
  GUI/Qt/Windows/LabelEditorDialog.h
  GUI/Qt/Windows/LayerInspectorDialog.h
  GUI/Qt/Windows/MainControlPanel.h
  GUI/Qt/Windows/MainImageWindow.h
  GUI/Qt/Windows/ReorientImageDialog.h
  GUI/Qt/Windows/SimpleFileDialogWithHistory.h
  GUI/Qt/Windows/SpeedImageDialog.h
  GUI/Qt/Windows/SplashPanel.h
  GUI/Qt/Windows/StatisticsDialog.h
)

# These UI headers don't need to be MOC'd
SET(UI_NONMOC_HEADERS
  GUI/Qt/Components/SNAPQtCommon.h
  GUI/Qt/Components/QtCursorOverride.h
  GUI/Qt/Components/QtRendererPlatformSupport.h
  GUI/Qt/Components/QtReporterDelegates.h
  GUI/Qt/Coupling/QtAbstractButtonCoupling.h
  GUI/Qt/Coupling/QtAbstractItemViewCoupling.h
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
  GUI/Qt/Testing/SNAPTestQt.h
)

SET(UI_FORMS
  GUI/Qt/Components/ColorMapInspector.ui
  GUI/Qt/Components/ContrastInspector.ui
  GUI/Qt/Components/CursorInspector.ui
  GUI/Qt/Components/DisplayLayoutInspector.ui
  GUI/Qt/Components/ImageInfoInspector.ui
  GUI/Qt/Components/LabelInspector.ui
  GUI/Qt/Components/MetadataInspector.ui
  GUI/Qt/Components/GeneralLayerInspector.ui
  GUI/Qt/Components/PaintbrushToolPanel.ui
  GUI/Qt/Components/QDoubleSliderWithEditor.ui
  GUI/Qt/Components/QtWarningDialog.ui
  GUI/Qt/Components/SliceViewPanel.ui
  GUI/Qt/Components/SnakeToolROIPanel.ui
  GUI/Qt/Components/SnakeWizardPanel.ui
  GUI/Qt/Components/SynchronizationInspector.ui
  GUI/Qt/Components/ViewPanel3D.ui
  GUI/Qt/Components/ZoomInspector.ui
  GUI/Qt/Windows/DropActionDialog.ui
  GUI/Qt/Windows/ImageIODialog.ui
  GUI/Qt/Windows/LabelEditorDialog.ui
  GUI/Qt/Windows/LayerInspectorDialog.ui
  GUI/Qt/Windows/MainControlPanel.ui
  GUI/Qt/Windows/MainImageWindow.ui
  GUI/Qt/Windows/ReorientImageDialog.ui
  GUI/Qt/Windows/SimpleFileDialogWithHistory.ui
  GUI/Qt/Windows/SpeedImageDialog.ui
  GUI/Qt/Windows/SplashPanel.ui
  GUI/Qt/Windows/StatisticsDialog.ui
)

SET(UI_RESOURCES
  GUI/Qt/Resources/SNAPResources.qrc
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
    ${OPENGL_INCLUDE_PATH}
    ${QT_QTSCRIPT_INCLUDE_DIR}
    ${QT_QTSCRIPTTOOLS_INCLUDE_DIR}
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
    ${SNAP_SOURCE_DIR}/GUI
    ${SNAP_SOURCE_DIR}/GUI/Model
    ${SNAP_SOURCE_DIR}/GUI/Renderer
    ${SNAP_SOURCE_DIR}/GUI/Renderer/OrientationWidget/Reorient
    ${SNAP_SOURCE_DIR}/GUI/Qt
    ${SNAP_SOURCE_DIR}/GUI/Qt/Components
    ${SNAP_SOURCE_DIR}/GUI/Qt/Coupling
    ${SNAP_SOURCE_DIR}/GUI/Qt/External
    ${SNAP_SOURCE_DIR}/GUI/Qt/External/ColorWheel
    ${SNAP_SOURCE_DIR}/GUI/Qt/Testing
    ${SNAP_SOURCE_DIR}/GUI/Qt/View
    ${SNAP_SOURCE_DIR}/GUI/Qt/Windows
    ${SNAP_SOURCE_DIR}/Testing
    ${SNAP_BINARY_DIR}
    ${OPENGL_INCLUDE_PATH}
    ${QT_QTSCRIPT_INCLUDE_DIR}
    ${QT_QTSCRIPTTOOLS_INCLUDE_DIR}
  )

ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 6" )

# Get rid of this ridiculous warning in VS8
IF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9" OR CMAKE_GENERATOR MATCHES "Visual Studio 10" )
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF( CMAKE_GENERATOR MATCHES "Visual Studio 8" OR CMAKE_GENERATOR MATCHES "Visual Studio 9" OR CMAKE_GENERATOR MATCHES "Visual Studio 10" )
  
IF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )
  ADD_DEFINITIONS(-DNOMINMAX)
ENDIF( CMAKE_GENERATOR MATCHES "^NMake" OR CMAKE_GENERATOR MATCHES "^Visual Studio" )


# ----------------------------------------------------------------
# Define External Libraries
# ----------------------------------------------------------------

# ITK Libraries
SET(SNAP_ITK_LIBS
  ${ITK_LIBRARIES}

)

# Core VTK libraries
SET(SNAP_VTK_CORE_LIBS
  vtkCharts
  vtkCommon
  vtkRendering
  vtkFiltering
  vtkGraphics
  vtkImaging
  vtkViews
  vtkIO
)

# VTK Libraries with possible inclusion of patented code
IF(VTK_USE_PATENTED)
  SET(SNAP_VTK_LIBS vtkPatented ${SNAP_VTK_CORE_LIBS})
ELSE(VTK_USE_PATENTED)
  SET(SNAP_VTK_LIBS ${SNAP_VTK_CORE_LIBS})
ENDIF(VTK_USE_PATENTED)

# Qt Libraries
SET(SNAP_QT_LIBS
  ${QT_LIBRARIES}
  ${QT_QTSCRIPT_LIBRARY}
  ${QT_QTSCRIPTTOOLS_LIBRARY}

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

# *****************************************************
# SNAP Logic and UI Libraries
# *****************************************************

# Wrap the QT input files
QT4_WRAP_UI(UI_FORM_HEADERS ${UI_FORMS})
QT4_WRAP_CPP(UI_HEADERS_MOC ${UI_MOC_HEADERS})
QT4_ADD_RESOURCES(UI_RESOURCES_RCC ${UI_RESOURCES})

# The SNAP logic library
ADD_LIBRARY(itksnaplogic ${LOGIC_CXX} ${LOGIC_HEADERS})

# The UI model library
ADD_LIBRARY(itksnapui_model ${UI_GENERIC_CXX} ${UI_GENERIC_HEADERS})

# The user interface code library
ADD_LIBRARY(itksnapui_qt
  ${UI_QT_CXX} ${UI_HEADERS_MOC} ${UI_MOC_HEADERS} ${UI_NONMOC_HEADERS} ${UI_FORM_HEADERS} ${UI_RESOURCES_RCC})

# This is experimental: it seems that shared libraries do not
# build accurately (at least on MacOS) without the following
# two lines
TARGET_LINK_LIBRARIES(itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui_model itksnaplogic ${SNAP_EXTERNAL_LIBS})
TARGET_LINK_LIBRARIES(itksnapui_qt ${SNAP_QT_LIBS} ${SNAP_EXTERNAL_LIBS})

# Designate the SNAP internal libraries
SET(SNAP_INTERNAL_LIBS itksnapui_qt itksnapui_model itksnaplogic)

# *****************************************************
# Define SNAP Executables
# *****************************************************
SET(SNAP_EXE InsightSNAP)

# Define the main SNAP executable
ADD_EXECUTABLE(${SNAP_EXE} GUI/Qt/main.cxx)
TARGET_LINK_LIBRARIES(${SNAP_EXE}
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS}
  )


ENABLE_TESTING()

MESSAGE(STATUS "--testQtScript 2")

ADD_TEST(TestQtScript ${SNAP_EXE} --testQtScript 2)
SET_TESTS_PROPERTIES(TestQtScript PROPERTIES PASS_REGULAR_EXPRESSION "QtScriptTest1::Run: 71")

# Define the testing EXE
### ADD_EXECUTABLE(snaptest ${TESTING_CXX})
### TARGET_LINK_LIBRARIES(snaptest
###   ${SNAP_INTERNAL_LIBS}
###   ${SNAP_EXTERNAL_LIBS})

SET(TEST_ORIENTATION_WIDGET_HEADERS
  #GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidget.h
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h
)

SET(TEST_ORIENTATION_WIDGET_CXX
  #GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidget.cxx
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.cxx
  GUI/Renderer/OrientationWidget/Test_OrientationWidget/main.cxx
)

QT4_WRAP_UI(UI_ORIENTATION_WIDGET_SRCS GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.ui)
QT4_WRAP_CPP(MOC_ORIENTATION_WIDGET_SRCS GUI/Renderer/OrientationWidget/Test_OrientationWidget/OrientationWidgetGUI.h)


ADD_EXECUTABLE(Test_OrientationWidget
  ${TEST_ORIENTATION_WIDGET_CXX} ${TEST_ORIENTATION_WIDGET_HEADERS}
  ${UI_ORIENTATION_WIDGET_SRCS} ${MOC_ORIENTATION_WIDGET_SRCS}
)

TARGET_LINK_LIBRARIES(Test_OrientationWidget
  ${SNAP_INTERNAL_LIBS}
  ${SNAP_EXTERNAL_LIBS}
)


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
IF(MSVC80 OR MSVC90 OR MSVC10)
  FIND_PROGRAM(VCREDIST_EXE vcredist_x86.exe vcredist_x64.exe)
  IF(VCREDIST_EXE)
    GET_FILENAME_COMPONENT(VCREDIST_NAME ${VCREDIST_EXE} NAME)
    INSTALL(FILES ${VCREDIST_EXE} DESTINATION bin)
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS 
      "ExecWait '\\\"$INSTDIR\\\\bin\\\\${VCREDIST_NAME}\\\" /q:a'")
  ENDIF(VCREDIST_EXE)
ENDIF(MSVC80 OR MSVC90 OR MSVC10)

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

SET(NIGHTLY_TARGET "qtsnap-nightly-${CPACK_SYSTEM_NAME}.${CPACK_EXTENSION}")

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

