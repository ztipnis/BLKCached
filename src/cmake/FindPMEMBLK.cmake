# The MIT License (MIT)
#
# Copyright (c) 2015 Justus Calvin
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

#
# FindPMEMBLK
# -------
#
# Find PMEMBLK include directories and libraries.
#
# Usage:
#
#  find_package(PMEMBLK [major[.minor]] [EXACT]
#               [QUIET] [REQUIRED]
#               [[COMPONENTS] [components...]]
#               [OPTIONAL_COMPONENTS components...]) 
#
# where the allowed components are PMEMBLKmalloc and PMEMBLK_preview. Users may modify 
# the behavior of this module with the following variables:
#
# * PMEMBLK_ROOT_DIR          - The base directory the of PMEMBLK installation.
# * PMEMBLK_INCLUDE_DIR       - The directory that contains the PMEMBLK headers files.
# * PMEMBLK_LIBRARY           - The directory that contains the PMEMBLK library files.
# * PMEMBLK_<library>_LIBRARY - The path of the PMEMBLK the corresponding PMEMBLK library. 
#                           These libraries, if specified, override the 
#                           corresponding library search results, where <library>
#                           may be PMEMBLK, PMEMBLK_debug, PMEMBLKmalloc, PMEMBLKmalloc_debug,
#                           PMEMBLK_preview, or PMEMBLK_preview_debug.
# * PMEMBLK_USE_DEBUG_BUILD   - The debug version of PMEMBLK libraries, if present, will
#                           be used instead of the release version.
#
# Users may modify the behavior of this module with the following environment
# variables:
#
# * PMEMBLK_INSTALL_DIR 
# * PMEMBLKROOT
# * LIBRARY_PATH
#
# This module will set the following variables:
#
# * PMEMBLK_FOUND             - Set to false, or undefined, if we haven’t found, or
#                           don’t want to use PMEMBLK.
# * PMEMBLK_<component>_FOUND - If False, optional <component> part of PMEMBLK sytem is
#                           not available.
# * PMEMBLK_VERSION           - The full version string
# * PMEMBLK_VERSION_MAJOR     - The major version
# * PMEMBLK_VERSION_MINOR     - The minor version
# * PMEMBLK_INTERFACE_VERSION - The interface version number defined in 
#                           PMEMBLK/PMEMBLK_stddef.h.
# * PMEMBLK_<library>_LIBRARY_RELEASE - The path of the PMEMBLK release version of 
#                           <library>, where <library> may be PMEMBLK, PMEMBLK_debug,
#                           PMEMBLKmalloc, PMEMBLKmalloc_debug, PMEMBLK_preview, or 
#                           PMEMBLK_preview_debug.
# * PMEMBLK_<library>_LIBRARY_DEGUG - The path of the PMEMBLK release version of 
#                           <library>, where <library> may be PMEMBLK, PMEMBLK_debug,
#                           PMEMBLKmalloc, PMEMBLKmalloc_debug, PMEMBLK_preview, or 
#                           PMEMBLK_preview_debug.
#
# The following varibles should be used to build and link with PMEMBLK:
#
# * PMEMBLK_INCLUDE_DIRS        - The include directory for PMEMBLK.
# * PMEMBLK_LIBRARIES           - The libraries to link against to use PMEMBLK.
# * PMEMBLK_LIBRARIES_RELEASE   - The release libraries to link against to use PMEMBLK.
# * PMEMBLK_LIBRARIES_DEBUG     - The debug libraries to link against to use PMEMBLK.
# * PMEMBLK_DEFINITIONS         - Definitions to use when compiling code that uses
#                             PMEMBLK.
# * PMEMBLK_DEFINITIONS_RELEASE - Definitions to use when compiling release code that
#                             uses PMEMBLK.
# * PMEMBLK_DEFINITIONS_DEBUG   - Definitions to use when compiling debug code that
#                             uses PMEMBLK.
#
# This module will also create the "PMEMBLK" target that may be used when building
# executables and libraries.

include(FindPackageHandleStandardArgs)

if(NOT PMEMBLK_FOUND)

  ##################################
  # Check the build type
  ##################################
  
  if(NOT DEFINED PMEMBLK_USE_DEBUG_BUILD)
    if(CMAKE_BUILD_TYPE MATCHES "(Debug|DEBUG|debug|RelWithDebInfo|RELWITHDEBINFO|relwithdebinfo)")
      set(PMEMBLK_BUILD_TYPE DEBUG)
    else()
      set(PMEMBLK_BUILD_TYPE RELEASE)
    endif()
  elseif(PMEMBLK_USE_DEBUG_BUILD)
    set(PMEMBLK_BUILD_TYPE DEBUG)
  else()
    set(PMEMBLK_BUILD_TYPE RELEASE)
  endif()
  
  ##################################
  # Set the PMEMBLK search directories
  ##################################
  
  # Define search paths based on user input and environment variables
  set(PMEMBLK_SEARCH_DIR ${PMEMBLK_ROOT_DIR} $ENV{PMEMBLK_INSTALL_DIR} $ENV{PMEMBLKROOT})
  
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux
    set(PMEMBLK_DEFAULT_SEARCH_DIR "/usr/local/lib/")
  else()
    message( FATAL_ERROR "PMDK not available via cmake on non-linux systems" )
  endif()
  
  ##################################
  # Find the PMEMBLK include dir
  ##################################
  
  find_path( 
      HINTS ${PMEMBLK_INCLUDE_DIR} ${PMEMBLK_SEARCH_DIR}
      PATHS ${PMEMBLK_DEFAULT_SEARCH_DIR}
      PATH_SUFFIXES include)

  ##################################
  # Set compile flags and libraries
  ##################################
    
  if(PMEMBLK_LIBRARIES_${PMEMBLK_BUILD_TYPE})
    set(PMEMBLK_DEFINITIONS "${PMEMBLK_DEFINITIONS_${PMEMBLK_BUILD_TYPE}}")
    set(PMEMBLK_LIBRARIES "${PMEMBLK_LIBRARIES_${PMEMBLK_BUILD_TYPE}}")
  elseif(PMEMBLK_LIBRARIES_RELEASE)
    set(PMEMBLK_DEFINITIONS "${PMEMBLK_DEFINITIONS_RELEASE}")
    set(PMEMBLK_LIBRARIES "${PMEMBLK_LIBRARIES_RELEASE}")
  elseif(PMEMBLK_LIBRARIES_DEBUG)
    set(PMEMBLK_DEFINITIONS "${PMEMBLK_DEFINITIONS_DEBUG}")
    set(PMEMBLK_LIBRARIES "${PMEMBLK_LIBRARIES_DEBUG}")
  endif()

  find_package_handle_standard_args(PMEMBLK 
      REQUIRED_VARS PMEMBLK_INCLUDE_DIRS PMEMBLK_LIBRARIES)

  ##################################
  # Create targets
  ##################################

  if(NOT CMAKE_VERSION VERSION_LESS 3.0 AND PMEMBLK_FOUND)
    add_library(PMEMBLK SHARED IMPORTED)
    set_target_properties(PMEMBLK PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES  ${PMEMBLK_INCLUDE_DIRS}
          IMPORTED_LOCATION              ${PMEMBLK_LIBRARIES})
  endif()

  mark_as_advanced(PMEMBLK_INCLUDE_DIRS PMEMBLK_LIBRARIES)

  unset(PMEMBLK_ARCHITECTURE)
  unset(PMEMBLK_BUILD_TYPE)
  unset(PMEMBLK_LIB_PATH_SUFFIX)
  unset(PMEMBLK_DEFAULT_SEARCH_DIR)

endif()
