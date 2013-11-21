# Locate SWRESAMPLE library
# This module defines
# SWRESAMPLE_LIBRARY, the name of the library to link against
# SWRESAMPLE_FOUND, if false, do not try to link to SWRESAMPLE
# SWRESAMPLE_INCLUDE_DIR, where to find libswresample/swresample.h
#

SET(SWRESAMPLE_FOUND "NO")

FIND_PATH(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h
  HINTS
  $ENV{SWRESAMPLE_ROOT}
  PATH_SUFFIXES include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw/include
)

#message("SWRESAMPLE_INCLUDE_DIR is ${SWRESAMPLE_INCLUDE_DIR}")

FIND_LIBRARY(SWRESAMPLE_LIBRARY
  NAMES swresample
  HINTS
  $ENV{SWRESAMPLE_ROOT}
  PATH_SUFFIXES lib64 lib bin
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

#message("SWRESAMPLE_LIBRARY is ${SWRESAMPLE_LIBRARY}")

SET(SWRESAMPLE_FOUND "YES")
