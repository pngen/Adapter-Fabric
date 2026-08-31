cmake_minimum_required(VERSION 3.24)

project(adapter_fabric
  VERSION 1.0.0
  DESCRIPTION "Adapter Fabric — model adapter authority and governance runtime"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Policy
# ---------------------------------------------------------------------------
if(POLICY CMP0155)
  cmake_policy(SET CMP0155 NEW)   # MSVC debug info
endif()
if(POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)   # link flags for static libs
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ADAPTER_FABRIC_BUILD_TESTS "Build the test suite" ON)
option(ADAPTER_FABRIC_BUILD_EXAMPLES "Build runnable examples" ON)
option(ADAPTER_FABRIC_BUILD_BENCHMARKS "Build benchmarks" ON)
option(ADAPTER_FABRIC_BUILD_CLI "Build the CLI / coordinator / worker executables" ON)
option(ADAPTER_FABRIC_USE_CUDA "Enable the CUDA accelerator backend" ON)
option(ADAPTER_FABRIC_BUILD_SHARED "Build a shared library instead of a static library" OFF)

set(ADAPTER_FABRIC_CUDA_ARCH "120" CACHE STRING "CUDA compute architecture (e.g. 120 for RTX 5090 / sm_120)")

if(ADAPTER_FABRIC_BUILD_SHARED)
  set(ADAPTER_FABRIC_LIB_TYPE SHARED)
else()
  set(ADAPTER_FABRIC_LIB_TYPE STATIC)
endif()

# ---------------------------------------------------------------------------
# Warning discipline (MSVC /W4 /WX, GCC/Clang matching flags)
# ---------------------------------------------------------------------------
function(af_enable_strict_warnings tgt)
  if(MSVC)
    target_compile_options(${tgt} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /permissive- /EHsc /Zc:preprocessor>)
    if(CMAKE_CUDA_COMPILER)
      target_compile_options(${tgt} PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W4 -Xcompiler=/WX -Xcompiler=/permissive->)
    endif()
  else()
    target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# CUDA
# ---------------------------------------------------------------------------
set(ADAPTER_FABRIC_HAS_CUDA FALSE)
set(ADAPTER_FABRIC_CUDA_SOURCES "")
if(ADAPTER_FABRIC_USE_CUDA)
  find_package(CUDAToolkit QUIET)
  if(CUDAToolkit_FOUND)
    enable_language(CUDA)
    set(ADAPTER_FABRIC_HAS_CUDA TRUE)
    set(ADAPTER_FABRIC_CUDA_SOURCES src/cuda/cuda_impl.cu)
  else()
    message(STATUS "ADAPTER_FABRIC: CUDAToolkit not found; building without the CUDA backend")
  endif()
endif()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
set(ADAPTER_FABRIC_SOURCES
  src/uuid.cpp
  src/checksum.cpp
  src/identity.cpp
  src/error.cpp
  src/adapter.cpp
  src/compatibility.cpp
  src/lifecycle.cpp
  src/residency.cpp
  src/capacity.cpp
  src/composition.cpp
  src/authority.cpp
  src/migration.cpp
  src/invalidation.cpp
  src/persistence.cpp
  src/explain.cpp
  src/inventory.cpp
  src/fabric.cpp
  src/protocol.cpp
  src/transport.cpp
  src/coordinator.cpp
  src/worker.cpp
  src/cuda/cuda_backend.cpp
  src/proof.cpp
  src/proof_cuda.cpp)

add_library(adapter_fabric ${ADAPTER_FABRIC_LIB_TYPE}
  ${ADAPTER_FABRIC_SOURCES}
  ${ADAPTER_FABRIC_CUDA_SOURCES})

add_library(adapter_fabric::adapter_fabric ALIAS adapter_fabric)

target_include_directories(adapter_fabric
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(adapter_fabric SYSTEM PUBLIC ${CUDAToolkit_INCLUDE_DIRS})

target_link_libraries(adapter_fabric
  PRIVATE
    $<$<BOOL:${ADAPTER_FABRIC_HAS_CUDA}>:CUDA::cudart>)

set_target_properties(adapter_fabric PROPERTIES
  OUTPUT_NAME adapter_fabric
  VERSION 1.0.0
  SOVERSION 1.0.0)

af_enable_strict_warnings(adapter_fabric)

target_compile_definitions(adapter_fabric PUBLIC
  ADAPTER_FABRIC_VERSION_MAJOR=1
  ADAPTER_FABRIC_VERSION_MINOR=0
  ADAPTER_FABRIC_VERSION_PATCH=0)
if(ADAPTER_FABRIC_HAS_CUDA)
  target_compile_definitions(adapter_fabric PUBLIC ADAPTER_FABRIC_HAS_CUDA=1)
endif()

if(ADAPTER_FABRIC_HAS_CUDA)
  set_property(TARGET adapter_fabric PROPERTY CUDA_ARCHITECTURES ${ADAPTER_FABRIC_CUDA_ARCH})
endif()

# ---------------------------------------------------------------------------
# Install + package export
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS adapter_fabric
  EXPORT AdapterFabricTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT AdapterFabricTargets
  FILE AdapterFabricTargets.cmake
  NAMESPACE adapter_fabric::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdapterFabric)

configure_package_config_file(
  cmake/AdapterFabricConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/AdapterFabricConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdapterFabric)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/AdapterFabricConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/AdapterFabricConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/AdapterFabricConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AdapterFabric)

install(FILES LICENSE NOTICE DESTINATION ${CMAKE_INSTALL_DATADIR}/adapter_fabric)

# ---------------------------------------------------------------------------
# CLI / coordinator / worker executables
# ---------------------------------------------------------------------------
if(ADAPTER_FABRIC_BUILD_CLI)
  foreach(app af_cli af_coordinator af_worker)
    add_executable(${app} apps/${app}.cpp)
    target_link_libraries(${app} PRIVATE adapter_fabric)
    af_enable_strict_warnings(${app})
  endforeach()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(ADAPTER_FABRIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
if(ADAPTER_FABRIC_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if(ADAPTER_FABRIC_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()