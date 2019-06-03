# - Check for the presence of libpmemblk
#
# The following variables are set when libpmemblk is found:
#  HAVE_libpmemblk       = Set to true, if all components of libpmemblk
#                          have been found.
#  libpmemblk_INCLUDES   = Include path for the header files of libpmemblk
#  libpmemblk_LIBRARIES  = Link these to use libpmemblk

## -----------------------------------------------------------------------------
## Check for the header files

find_path (libpmemblk_INCLUDES libpmemblk.h
  PATHS /usr/local/include /usr/include /sw/include
  )
  find_path (pmemblk_INCLUDE_DIRS libpmemblk.h
  PATHS /usr/local/include /usr/include /sw/include
  )

## -----------------------------------------------------------------------------
## Check for the library

find_library (libpmemblk_LIBRARIES pmemblk
  PATHS /usr/local/lib /usr/lib /lib /sw/lib
  )
find_library (pmemblk_LIBRARIES pmemblk
  PATHS /usr/local/lib /usr/lib /lib /sw/lib
  )

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (libpmemblk_INCLUDES AND libpmemblk_LIBRARIES)
  set (HAVE_libpmemblk TRUE)
else (libpmemblk_INCLUDES AND libpmemblk_LIBRARIES)
  if (NOT libpmemblk_FIND_QUIETLY)
    if (NOT libpmemblk_INCLUDES)
      message (STATUS "Unable to find libpmemblk header files!")
    endif (NOT libpmemblk_INCLUDES)
    if (NOT libpmemblk_LIBRARIES)
      message (STATUS "Unable to find libpmemblk library files!")
    endif (NOT libpmemblk_LIBRARIES)
  endif (NOT libpmemblk_FIND_QUIETLY)
endif (libpmemblk_INCLUDES AND libpmemblk_LIBRARIES)

if (HAVE_libpmemblk)
  if (NOT libpmemblk_FIND_QUIETLY)
    message (STATUS "Found components for libpmemblk")
    message (STATUS "libpmemblk_INCLUDES = ${libpmemblk_INCLUDES}")
    message (STATUS "libpmemblk_LIBRARIES     = ${libpmemblk_LIBRARIES}")
  endif (NOT libpmemblk_FIND_QUIETLY)
else (HAVE_libpmemblk)
  if (libpmemblk_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find libpmemblk!")
  endif (libpmemblk_FIND_REQUIRED)
endif (HAVE_libpmemblk)

mark_as_advanced (
  HAVE_libpmemblk
  libpmemblk_LIBRARIES
  libpmemblk_INCLUDES
  )