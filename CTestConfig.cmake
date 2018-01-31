## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
##
## # The following are required to submit to the CDash dashboard:
##   ENABLE_TESTING()
##   INCLUDE(CTest)

set(CTEST_PROJECT_NAME "ITK-SNAP 3.4")
set(CTEST_NIGHTLY_START_TIME "00:00:00 UTC")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "itksnap.org")
set(CTEST_DROP_LOCATION "/cdash/submit.php?project=ITK-SNAP+3.4")
set(CTEST_DROP_SITE_CDASH TRUE)
