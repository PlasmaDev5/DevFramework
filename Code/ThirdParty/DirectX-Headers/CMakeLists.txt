pl_cmake_init()

include(GNUInstallDirs)

# Enables consumers to add this library as a link target to automatically add
# these include directories, regardless of whether this is referenced via subdirectory
# or from an installed location

pl_create_target(INTERFACE DirectX-Headers)
target_include_directories(DirectX-Headers SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# For non-Windows targets, also add the WSL stubs to the include path
if (NOT WIN32)
    target_include_directories(DirectX-Headers SYSTEM INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/wsl/stubs>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/wsl/stubs>"
    )
endif()

add_library(Microsoft::DirectX-Headers ALIAS DirectX-Headers)