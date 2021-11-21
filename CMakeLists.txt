cmake_minimum_required (VERSION 3.14)
project(shmem C)

set(PUREDATA_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/pure-data/src" CACHE PATH "Path to folder containing m_pd.h" )
if(WIN32)
  set(PUREDATA_LIB "${CMAKE_SOURCE_DIR}/pure-data/bin/pd.lib" CACHE PATH "Path to pd.lib for Windows")
endif()

include_directories(${PUREDATA_INCLUDE_DIRS} ${CMAKE_BINARY_DIR})

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/package" CACHE PATH "Package destination folder" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang$")
  set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif()

# Get git info
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(git_info)
get_commit_sha(GIT_SHA)
get_commit_date(GIT_DATE)
get_tag(GIT_TAG)
configure_file("${CMAKE_SOURCE_DIR}/shmem_version.c.in" "${CMAKE_BINARY_DIR}/shmem_version.c" @ONLY)

add_library(${CMAKE_PROJECT_NAME} SHARED shmem.c shmem_version.h "${CMAKE_BINARY_DIR}/shmem_version.c")

if(WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${PUREDATA_LIB}")
endif()

# set library binary file name and extension
set_target_properties(${CMAKE_PROJECT_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${OBJECT_NAME}")

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    )

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES DEFINE_SYMBOL "SHMEM_EXPORTS")

if(APPLE)
  set(PLATFORM_SUFFIX "d") # d for Darwin
elseif(UNIX)
  set(PLATFORM_SUFFIX "l") # l for Linux
else()
  set(PLATFORM_SUFFIX "m") # m for Microsoft
endif()

if(CMAKE_GENERATOR_PLATFORM MATCHES Win32)
  set(ARCH_SUFFIX i386)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*arm.*")
  set(ARCH_SUFFIX "arm")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*aarch64.*")
  set(ARCH_SUFFIX "aarch64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*64.*")
  set(ARCH_SUFFIX "amd64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES ".*86.*")
  set(ARCH_SUFFIX "i386")
else()
  message(FATAL "Could not determine target architecture")
endif()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".${PLATFORM_SUFFIX}_${ARCH_SUFFIX}" PREFIX "")

if(APPLE)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE -Wl,-undefined,dynamic_lookup)
endif()

install(TARGETS ${CMAKE_PROJECT_NAME}
    DESTINATION ${CMAKE_PROJECT_NAME}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)


include(GenerateExportHeader)
generate_export_header(${CMAKE_PROJECT_NAME})

install(DIRECTORY examples/ 
  DESTINATION ${CMAKE_PROJECT_NAME}/examples
  FILES_MATCHING PATTERN "*.pd")

FILE(GLOB HELP_FILES "${PROJECT_SOURCE_DIR}/*.pd")
  
INSTALL(FILES ${HELP_FILES}
        DESTINATION ${CMAKE_PROJECT_NAME})

install(FILES 
  LICENSE.txt
  README.md
  DESTINATION ${CMAKE_PROJECT_NAME}
)
