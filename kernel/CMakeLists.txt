cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(remaPSV2_kernel)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_definitions(-DRELEASE) 

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
  src/main.c
  src/userspace.c
  src/profile.c
  src/ini.c
  src/common.c
  src/remap.c
  src/ui.c
  src/ui-control.c
  src/ui-draw.c
  src/renderer.c
  src/log.c
)

target_link_libraries(${PROJECT_NAME}
  k
  gcc
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceThreadmgrForDriver_stub
  SceSysmemForDriver_stub
  SceSysrootForKernel_stub
  SceAppMgrForDriver_stub
  SceCtrlForDriver_stub
  SceIofilemgrForDriver_stub
  SceTouchForDriver_stub
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

vita_create_self(remaPSV2.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG export.yml
)
add_dependencies(remaPSV2.skprx ${PROJECT_NAME})
 
vita_create_stubs(stubs ${PROJECT_NAME} export.yml KERNEL)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)
