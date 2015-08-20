# --------------------------------------------------------------------------
# PlusApp
SET(PLUSBUILD_SVN_REVISION_ARGS)
IF ( NOT PLUS_SVN_REVISION STREQUAL "0" )
  SET(PLUSBUILD_SVN_REVISION_ARGS 
    SVN_REVISION -r "${PLUS_SVN_REVISION}"
    )
ENDIF()

SET(PLUSBUILD_ADDITIONAL_SDK_ARGS)

IF ( PLUSBUILD_DOCUMENTATION )
  SET(PLUSBUILD_ADDITIONAL_SDK_ARGS ${PLUSBUILD_ADDITIONAL_SDK_ARGS} 
    -DPLUSAPP_DOCUMENTATION_SEARCH_SERVER_INDEXED=${PLUSBUILD_DOCUMENTATION_SEARCH_SERVER_INDEXED}
    -DDOXYGEN_DOT_EXECUTABLE:FILEPATH=${DOXYGEN_DOT_EXECUTABLE}
    -DDOXYGEN_EXECUTABLE:FILEPATH=${DOXYGEN_EXECUTABLE}
    )
ENDIF()

SET (PLUS_PLUSAPP_DIR ${CMAKE_BINARY_DIR}/PlusApp CACHE INTERNAL "Path to store PlusApp contents.")
ExternalProject_Add(PlusApp
  SOURCE_DIR "${PLUS_PLUSAPP_DIR}" 
  BINARY_DIR "PlusApp-bin"
  #--Download step--------------
  SVN_USERNAME ${PLUSBUILD_ASSEMBLA_USERNAME}
  SVN_PASSWORD ${PLUSBUILD_ASSEMBLA_PASSWORD}
  SVN_REPOSITORY https://subversion.assembla.com/svn/plus/branches/qt5/PlusApp
  ${PLUSBUILD_SVN_REVISION_ARGS}
  #--Configure step-------------
  CMAKE_ARGS 
    ${ep_common_args}
    -DPLUS_EXECUTABLE_OUTPUT_PATH:STRING=${PLUS_EXECUTABLE_OUTPUT_PATH}
    -DPLUSLIB_DIR:PATH=${PLUSLIB_DIR}
    -DSubversion_SVN_EXECUTABLE:FILEPATH=${Subversion_SVN_EXECUTABLE}
    -DPLUSAPP_BUILD_DiagnosticTools:BOOL=ON
    -DPLUSAPP_BUILD_fCal:BOOL=ON
    -DPLUSAPP_TEST_GUI:BOOL=${PLUSAPP_TEST_GUI}
    -DBUILD_DOCUMENTATION:BOOL=${PLUSBUILD_DOCUMENTATION}
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    -DPLUSAPP_PACKAGE_EDITION:STRING=${PLUSAPP_PACKAGE_EDITION}
    -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    ${PLUSBUILD_ADDITIONAL_SDK_ARGS}
  #--Build step-----------------
  BUILD_ALWAYS 1
  #--Install step-----------------
  INSTALL_COMMAND ""
  DEPENDS ${PlusApp_DEPENDENCIES}
  )
SET(PLUSAPP_DIR ${CMAKE_BINARY_DIR}/PlusApp-bin CACHE PATH "The directory containing PlusApp binaries" FORCE)                

# --------------------------------------------------------------------------
# Copy Qt binaries to PLUS_EXECUTABLE_OUTPUT_PATH
IF( ${QT_VERSION_MAJOR} EQUAL 5 )
  SET(RELEASE_REGEX_PATTERN Qt5.*[^d]${CMAKE_SHARED_LIBRARY_SUFFIX})
  SET(DEBUG_REGEX_PATTERN Qt5.*d${CMAKE_SHARED_LIBRARY_SUFFIX})
ELSEIF( ${QT_VERSION_MAJOR} EQUAL 4 )
  SET(RELEASE_REGEX_PATTERN .*[^d]4${CMAKE_SHARED_LIBRARY_SUFFIX} )
  SET(DEBUG_REGEX_PATTERN .*d4${CMAKE_SHARED_LIBRARY_SUFFIX} )
ENDIF()

IF ( ${CMAKE_GENERATOR} MATCHES "Visual Studio" )
  FILE(COPY "${QT_BINARY_DIR}/"
    DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}/Release
    FILES_MATCHING REGEX ${RELEASE_REGEX_PATTERN}
    )
  FILE(COPY "${QT_BINARY_DIR}/"
    DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}/Debug
    FILES_MATCHING REGEX ${DEBUG_REGEX_PATTERN}
    )
ELSE()
  FILE(COPY "${QT_BINARY_DIR}/"
    DESTINATION ${PLUS_EXECUTABLE_OUTPUT_PATH}
    FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
ENDIF()
