FIND_PACKAGE(CURL)

IF(CURL_FOUND)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
ENDIF(CURL_FOUND)

ADD_EXECUTABLE(itksnap-wt WorkspaceTool.cxx)
TARGET_LINK_LIBRARIES(itksnap-wt itksnaplogic ${ITK_LIBRARIES} ${CURL_LIBRARIES})

# Install the workspace tool
SNAP_INSTALL_BINARY(itksnap-wt itksnap-wt "")

