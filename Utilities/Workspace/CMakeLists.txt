# Add the exe for the workspace tool. On Windows it gets the same manifest as ITK-SNAP, so that
# non-ASCII paths on its command line reach it as UTF-8 (SNAP_WIN32_MANIFEST is empty elsewhere)
ADD_EXECUTABLE(itksnap-wt WorkspaceTool.cxx ${SNAP_WIN32_MANIFEST})
TARGET_LINK_LIBRARIES(itksnap-wt itksnaplogic ${ITK_LIBRARIES} ${CURL_LIBRARIES})

# Install the workspace tool
INSTALL(TARGETS itksnap-wt DESTINATION ${SNAP_CLI_INSTALL_PATH} COMPONENT Runtime)

