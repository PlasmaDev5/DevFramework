pl_cmake_init()

pl_requires(PL_CMAKE_PLATFORM_WINDOWS)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE DIRECTX_SUPPORT=1)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RHI
  d3d12
  dxgi
  dxguid
  DirectX-Headers
)
