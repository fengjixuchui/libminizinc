# - Try to find Gecode
# Once done this will define
#  GECODE_FOUND          - System has Gecode
#  GECODE_INCLUDE_DIRS   - The Gecode include directories
#  GECODE_LIBRARIES      - The libraries needed to use Gecode
#  GECODE_TARGETS        - The names of imported targets created for gecode
# User can set GECODE_ROOT to the preferred installation prefix

find_path(GECODE_INCLUDE gecode/kernel.hh
          HINTS ${GECODE_ROOT} ENV GECODE_ROOT
          PATH_SUFFIXES include)

set(GECODE_REQ_LIBS gecodedriver gecodefloat gecodegist gecodeint gecodekernel gecodeminimodel gecodesearch gecodeset gecodesupport)

foreach(GECODE_LIB ${GECODE_REQ_LIBS})
  # Determine windows library name
  file(GLOB_RECURSE GECODE_LIB_WIN RELATIVE "${GECODE_INCLUDE}/../lib" "${GECODE_LIB}*.lib")
  if(GECODE_LIB_WIN)
    get_filename_component(GECODE_LIB_WIN ${GECODE_LIB_WIN} NAME_WE)
  endif()

  # Try to find gecode library
  set(GECODE_LIB_LOC "GECODE_LIB_LOC-NOTFOUND")
  find_library(GECODE_LIB_LOC NAMES ${GECODE_LIB} ${GECODE_LIB_WIN}
               HINTS ${GECODE_ROOT} ENV GECODE_ROOT
               PATH_SUFFIXES lib)
  if("${GECODE_LIB_LOC}" STREQUAL "GECODE_LIB_LOC-NOTFOUND" AND NOT "${GECODE_LIB}" STREQUAL "gecodegist")
#    message(STATUS "Gecode: Could not find library `${GECODE_LIB}`")
    set(GECODE_LIBRARY "")
    break()
  endif()
  if(NOT "${GECODE_LIB_LOC}" STREQUAL "GECODE_LIB_LOC-NOTFOUND")
    list(APPEND GECODE_LIBRARY ${GECODE_LIB_LOC})
    if(NOT (SHARED_GECODE AND WIN32) )
      add_library(${GECODE_LIB} UNKNOWN IMPORTED)
      set_target_properties(${GECODE_LIB} PROPERTIES
                            IMPORTED_LOCATION ${GECODE_LIB_LOC}
                            INTERFACE_INCLUDE_DIRECTORIES ${GECODE_INCLUDE})
      list(APPEND GECODE_TARGETS ${GECODE_LIB})
    endif()
  endif()
endforeach(GECODE_LIB)

if(SHARED_GECODE AND WIN32)
  get_filename_component(GECODE_LIB_WIN ${GECODE_LIB_LOC} DIRECTORY)
  link_directories(${GECODE_LIB_WIN})
endif()

unset(GECODE_REQ_LIBS)
unset(GECODE_LIB_WIN)
unset(GECODE_LIB_LOC)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GECODE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Gecode DEFAULT_MSG
                                  GECODE_INCLUDE GECODE_LIBRARY)

mark_as_advanced(GECODE_INCLUDE GECODE_LIBRARY)

set(GECODE_LIBRARIES ${GECODE_LIBRARY})
set(GECODE_INCLUDE_DIRS ${GECODE_INCLUDE})