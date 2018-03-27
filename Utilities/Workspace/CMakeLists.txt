# Add the exe for the workspace tool
ADD_EXECUTABLE(itksnap-wt WorkspaceTool.cxx)
TARGET_LINK_LIBRARIES(itksnap-wt itksnaplogic ${ITK_LIBRARIES} ${CURL_LIBRARIES})

# Install the workspace tool
INSTALL(TARGETS itksnap-wt DESTINATION ${SNAP_CLI_INSTALL_PATH} COMPONENT Runtime)

