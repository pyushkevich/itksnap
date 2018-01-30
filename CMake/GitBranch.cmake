# This code assigns the git branch to a variable
function(get_git_branch RESULTNAME)

  # Find Git and its libraries
  if(NOT GIT_FOUND)
    find_package(Git QUIET)
  endif(NOT GIT_FOUND)

  if(GIT_FOUND)

    # Call git to get branch id
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE SNAP_VERSION_GIT_BRANCH
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(${RESULTNAME} ${SNAP_VERSION_GIT_BRANCH} PARENT_SCOPE)

  endif(GIT_FOUND)

endfunction()

