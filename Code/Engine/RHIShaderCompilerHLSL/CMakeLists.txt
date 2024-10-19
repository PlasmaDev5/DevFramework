pl_cmake_init()

set(VULKAN_SUPPORT ON)
set(DIRECTX_SUPPORT ON)
set(METAL_SUPPORT ON)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

set(RSCH_HEADERS Compiler.h DXCLoader.h ShaderReflection.h RHIShaderCompilerHLSLDLL.h)
set(RSCH_SOURCES Compiler.cpp DXCLoader.cpp ShaderReflection.cpp)

include_directories()

if (VULKAN_SUPPORT OR METAL_SUPPORT)
  list(APPEND RSCH_HEADERS SPIRVReflection.h)
  list(APPEND RSCH_SOURCES SPIRVReflection.cpp)
endif()

if (METAL_SUPPORT)
  list(APPEND RSCH_HEADERS MSLConverter.h)
  list(APPEND RSCH_SOURCES MSLConverter.cpp)
endif()

if (DIRECTX_SUPPORT)
  list(APPEND RSCH_HEADERS DXILReflection.h)
  list(APPEND RSCH_SOURCES DXILReflection.cpp)
endif()

set(RSCH_MANUAL_SOURCES ${RSCH_HEADERS} ${RSCH_SOURCES})

pl_create_target(LIBRARY ${PROJECT_NAME} MANUAL_SOURCE_FILES ${RSCH_MANUAL_SOURCES})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
endif()

if (VULKAN_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_SUPPORT=1)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spirv-cross-core
    spirv-cross-hlsl
  )
endif()

if (METAL_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PRIVATE METAL_SUPPORT=1)
  target_link_libraries(${PROJECT_NAME}
          PRIVATE
          spirv-cross-core
          spirv-cross-hlsl
          spirv-cross-msl
          )
endif()

if (DIRECTX_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PRIVATE DIRECTX_SUPPORT=1)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    d3d12
    dxgi
    dxguid
    dia
  )
endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RHI
  d3dcompiler
  dxc
)