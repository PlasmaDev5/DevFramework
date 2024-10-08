pl_cmake_init()

pl_requires(PL_3RDPARTY_ZLIB_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

if(PL_CMAKE_PLATFORM_WINDOWS_UWP OR PL_CMAKE_PLATFORM_POSIX)
  pl_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_PL_PREFIX NO_WARNINGS_AS_ERRORS)
else()
  pl_create_target(LIBRARY ${PROJECT_NAME} NO_PL_PREFIX NO_WARNINGS_AS_ERRORS)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZLIB_SUPPORT)

if(PL_CMAKE_PLATFORM_WINDOWS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _CRT_SECURE_NO_WARNINGS)

  if(NOT PL_CMAKE_PLATFORM_WINDOWS_UWP AND PL_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ZLIB_DLL=1)
  endif()

  target_compile_options(${PROJECT_NAME} PRIVATE /wd4131 /wd4996)
endif()
