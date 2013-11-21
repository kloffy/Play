# Locate SWSCALE library
# This module defines
# SWSCALE_LIBRARY, the name of the library to link against
# SWSCALE_FOUND, if false, do not try to link to SWSCALE
# SWSCALE_INCLUDE_DIR, where to find libswscale/swscale.h
#

SET(SWSCALE_FOUND "NO")

FIND_PATH(SWSCALE_INCLUDE_DIR libswscale/swscale.h
  HINTS
  $ENV{SWSCALE_ROOT}
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

#message("SWSCALE_INCLUDE_DIR is ${SWSCALE_INCLUDE_DIR}")

FIND_LIBRARY(SWSCALE_LIBRARY
  NAMES swscale
  HINTS
  $ENV{SWSCALE_ROOT}
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

#message("SWSCALE_LIBRARY is ${SWSCALE_LIBRARY}")

SET(SWSCALE_FOUND "YES")
