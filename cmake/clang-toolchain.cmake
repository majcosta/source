# Set the target platform to Windows.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0.22000.0)

set(triple i386-pc-win32-msvc)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_RC_COMPILER llvm-rc)

set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_RC_COMPILER_TARGET ${triple})

set(msvc_dir /home/marco/vms/shared)
set(msvc_include
       "${msvc_dir}/14.34.31933/include"
       "${msvc_dir}/14.34.31933/ATLMFC/include"
       "${msvc_dir}/include/10.0.22000.0/ucrt"
       "${msvc_dir}/include/10.0.22000.0/um"
       "${msvc_dir}/include/10.0.22000.0/shared"
       "${msvc_dir}/include/10.0.22000.0/winrt"
       "${msvc_dir}/include/10.0.22000.0/cppwinrt"
)
set(msvc_libraries
       "${msvc_dir}/14.34.31933/ATLMFC/lib/x86"
       "${msvc_dir}/14.34.31933/lib/x86"
       "${msvc_dir}/lib/10.0.22000.0/ucrt/x86"
       "${msvc_dir}/lib/10.0.22000.0/um/x86"
)

foreach(LANG C CXX)
       set(CMAKE_${LANG}_STANDARD_INCLUDE_DIRECTORIES ${msvc_include})
endforeach()

add_compile_definitions(WIN32)
link_directories(${msvc_libraries})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#add_link_options("-municode")
