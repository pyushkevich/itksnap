#############################################
# REQUIRE ITK 5.4 OR LATER                 #
#############################################
FIND_PACKAGE(ITK 5.4 REQUIRED COMPONENTS
  ITKAnisotropicSmoothing
  ITKAntiAlias
  ITKBiasCorrection
  ITKBinaryMathematicalMorphology
  ITKColormap
  ITKCommon
  ITKConnectedComponents
  ITKConvolution
  ITKDisplacementField
  ITKDistanceMap
  ITKFFT
  ITKFiniteDifference
  ITKGDCM
  ITKGPUCommon
  ITKGPUSmoothing
  ITKIOGDCM
  ITKIOGE
  ITKIOGIPL
  ITKIOImageBase
  ITKIOMeshBase
  ITKIOMeta
  ITKIONIFTI
  ITKIONRRD
  ITKIORAW
  ITKIOSiemens
  ITKIOTransformBase
  ITKIOVTK
  ITKIOXML
  ITKImageAdaptors
  ITKImageCompare
  ITKImageCompose
  ITKImageFeature
  ITKImageFilterBase
  ITKImageFunction
  ITKImageGradient
  ITKImageGrid
  ITKImageIntensity
  ITKImageLabel
  ITKImageNoise
  ITKImageStatistics
  ITKLabelMap
  ITKLabelVoting
  ITKLevelSets
  ITKMathematicalMorphology
  ITKMesh
  ITKRegistrationCommon
  ITKSmoothing
  ITKSpatialObjects
  ITKStatistics
  ITKTestKernel
  ITKThresholding
  ITKTransform
  ITKTransformFactory
  ITKVTK
  ITKWatersheds
  ITKZLIB
  ITKImageIO
  ITKMeshIO
  ITKNIFTI
  ITKTransformIO
  MorphologicalContourInterpolation)

INCLUDE(${ITK_USE_FILE})

#############################################
# REQUIRE VTK                               #
#############################################
FIND_PACKAGE(VTK 9.3.1 REQUIRED COMPONENTS
  ChartsCore
  CommonComputationalGeometry
  CommonCore
  CommonDataModel
  CommonExecutionModel
  CommonMath
  CommonTransforms
  FiltersCore
  FiltersGeneral
  FiltersGeometry
  FiltersSources
  GUISupportQt
  IOExport
  IOGeometry
  IOImage
  IOLegacy
  IOPLY
  ImagingCore
  ImagingGeneral
  InteractionStyle
  InteractionWidgets
  RenderingAnnotation
  RenderingContext2D
  RenderingContextOpenGL2
  RenderingCore
  RenderingLOD
  RenderingOpenGL2
  RenderingUI
  RenderingVolume
  RenderingVolumeOpenGL2
  RenderingGL2PSOpenGL2
  ViewsContext2D)

#############################################
# REQUIRE QT6                               #
#############################################
FIND_PACKAGE(Qt6Widgets)
FIND_PACKAGE(Qt6OpenGL)
FIND_PACKAGE(Qt6Concurrent)
FIND_PACKAGE(Qt6Qml)

SET(SNAP_QT_INCLUDE_DIRS
  ${Qt6Widgets_INCLUDE_DIRS}
  ${Qt6OpenGL_INCLUDE_DIRS}
  ${Qt6Concurrent_INCLUDE_DIRS}
  ${Qt6Qml_INCLUDE_DIRS}
)

SET(SNAP_QT_LIBRARIES
  Qt6::Widgets
  Qt6::OpenGL
  Qt6::Concurrent
  Qt6::Qml
)

# On Linux the X11Extras library is required
# IF(UNIX AND NOT APPLE)
#   FIND_PACKAGE(Qt6X11Extras)
#   SET(SNAP_QT_INCLUDE_DIRS ${SNAP_QT_INCLUDE_DIRS} ${Qt6X11Extras_INCLUDE_DIRS})
#   SET(SNAP_QT_LIBRARIES ${SNAP_QT_LIBRARIES} Qt6::X11Extras)
# ENDIF()

# Set vars for the QT binary and library directories
GET_FILENAME_COMPONENT(QT_BINARY_DIR "${Qt6Core_DIR}/../../../bin" ABSOLUTE)
GET_FILENAME_COMPONENT(QT_LIBRARY_DIR "${Qt6Core_DIR}/../../" ABSOLUTE)

# Set the QTVERSION var
SET(QTVERSION ${Qt6Widgets_VERSION})

# Look for CURL. It is now required part of ITK-SNAP
FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
ENDIF(CURL_FOUND)


