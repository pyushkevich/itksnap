#############################################
# REQUIRE ITK 3.20 OR LATER                 #
#############################################
FIND_PACKAGE(ITK 5.1.2 REQUIRED COMPONENTS
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
  ITKTransformIO
  MorphologicalContourInterpolation)

INCLUDE(${ITK_USE_FILE})

#############################################
# REQUIRE VTK                               #
#############################################
FIND_PACKAGE(VTK 8.2 REQUIRED)
INCLUDE (${VTK_USE_FILE})

#############################################
# REQUIRE QT5                               #
#############################################
FIND_PACKAGE(Qt5Widgets)
FIND_PACKAGE(Qt5OpenGL)
FIND_PACKAGE(Qt5Concurrent)
FIND_PACKAGE(Qt5Qml)

SET(SNAP_QT_INCLUDE_DIRS
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5OpenGL_INCLUDE_DIRS}
  ${Qt5Concurrent_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS}
)

SET(SNAP_QT_LIBRARIES
  Qt5::Widgets
  Qt5::OpenGL
  Qt5::Concurrent
  Qt5::Qml
)

# Set vars for the QT binary and library directories
GET_FILENAME_COMPONENT(QT_BINARY_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
GET_FILENAME_COMPONENT(QT_LIBRARY_DIR "${Qt5Core_DIR}/../../" ABSOLUTE)

# Set the QTVERSION var
SET(QTVERSION ${Qt5Widgets_VERSION})

# Look for CURL. It is now required part of ITK-SNAP
FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
ENDIF(CURL_FOUND)


