# - Find HDF5 (includes and libraries)
#
# This module defines
#  HDF5_INCLUDE_DIR
#  HDF5_LIBRARIES
#  HDF5_HL_LIBRARIES
#  HDF5_FOUND
#

set(HDF5_FIND_REQUIRED true)

find_path(HDF5_INCLUDE_DIR hdf5.h
    /usr/include/
    /usr/local/include/
)

set(HDF5_NAMES ${HDF5_NAMES} hdf5 )
set(HDF5_HL_NAMES hdf5_hl )

find_library(HDF5_LIBRARY
    NAMES ${HDF5_NAMES}
    PATHS
    /usr/lib64/
    /usr/lib/
    /usr/local/lib64/
    /usr/local/lib/
)

message(STATUS "HDF5_HL names: " ${HDF5_HL_NAMES})

find_library(HDF5_HL_LIBRARY
    NAMES ${HDF5_HL_NAMES}
    PATHS
    /usr/lib64/
    /usr/lib/
    /usr/local/lib64/
    /usr/local/lib/
)


if (HDF5_LIBRARY AND HDF5_INCLUDE_DIR AND HDF5_HL_LIBRARY )
    set(HDF5_LIBRARIES ${HDF5_LIBRARY})
    set(HDF5_HL_LIBRARIES ${HDF5_HL_LIBRARY})
    set(HDF5_FOUND true)
endif (HDF5_LIBRARY AND HDF5_INCLUDE_DIR AND HDF5_HL_LIBRARY)



# Hide in the cmake cache
mark_as_advanced(HDF5_LIBRARIES)
