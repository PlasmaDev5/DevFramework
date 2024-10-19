pl_cmake_init()

pl_create_target(INTERFACE dxc)
target_include_directories(dxc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(dxc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/dxc")
target_compile_definitions(dxc INTERFACE DXC_CUSTOM_LOCATION="${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(dxc_default_location "C:/Program Files (x86)/Windows Kits/10/Bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")
target_compile_definitions(dxc INTERFACE DXC_DEFAULT_LOCATION="${dxc_default_location}")

find_package(DiaSDK REQUIRED)
pl_create_target(INTERFACE dia)
target_include_directories(dia INTERFACE "${DIASDK_INCLUDE_DIRS}")