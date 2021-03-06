INCLUDE(FindPackageHandleStandardArgs)
INCLUDE(PackageManagerPaths)

FIND_PATH(CHOLMOD_INCLUDES NAMES cholmod.h 
                          PATHS ${PACKMAN_INCLUDE_PATHS} 
                          PATH_SUFFIXES suitesparse)

FIND_LIBRARY(CHOLMOD_LIBRARIES NAMES cholmod 
                              PATHS ${PACKMAN_LIBRARIES_PATHS})

IF(CHOLMOD_INCLUDES AND CHOLMOD_LIBRARIES)
  SET(CHOLMOD_FOUND True)
ENDIF(CHOLMOD_INCLUDES AND CHOLMOD_LIBRARIES)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CHOLDMOD DEFAULT_MSG CHOLMOD_INCLUDES CHOLMOD_LIBRARIES)


