PROJECT(SNAP_LAUNCHER)

# -- EXPERIMENTAL --
IF(NOT WIN32 AND NOT APPLE)
  SET(SNAP_EXE_INSTALL ${SNAP_MAIN_INSTALL_DIR})
  SET(SNAP_FORWARD_DIR_BUILD "${CMAKE_BINARY_DIR}/bin")
  SET(SNAP_FORWARD_DIR_INSTALL "../${SNAP_EXE_INSTALL}")
  SET(SNAP_FORWARD_PATH_BUILD "\"${SNAP_FORWARD_DIR_BUILD}\"")
  SET(SNAP_FORWARD_PATH_INSTALL "\"${SNAP_FORWARD_DIR_INSTALL}\"")
  SET(SNAP_FORWARD_EXE ${SNAP_EXE})
  SET(KWSYS_NAMESPACE itksys)
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/Utilities/Forwarding/SharedForward.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/SharedForward.h
    @ONLY IMMEDIATE)
  CONFIGURE_FILE(
    ${SNAP_SOURCE_DIR}/Utilities/Forwarding/SharedForwardExe.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/shared-forward.c
    @ONLY IMMEDIATE)
  ADD_EXECUTABLE(itksnap ${CMAKE_CURRENT_BINARY_DIR}/shared-forward.c)
  ADD_DEPENDENCIES(itksnap ${SNAP_EXE})
  INSTALL(TARGETS itksnap RUNTIME DESTINATION bin)
ENDIF(NOT WIN32 AND NOT APPLE)

