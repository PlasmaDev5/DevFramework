
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CURRENT_OSX_VERSION)

    set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_NAME "OSX")
    set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_PREFIX "Osx")
    set_property(GLOBAL PROPERTY PL_CMAKE_PLATFORM_POSTFIX "OSX")

endif()