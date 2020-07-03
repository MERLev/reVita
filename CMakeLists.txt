cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(remaPSV2)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -DTAIPOOL_AS_STDLIB")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_definitions(-DRELEASE)

add_executable(remaPSV2
  main.c
  renderer.c
)

target_link_libraries(remaPSV2
  taipool
  taihen_stub
  k
  gcc
  SceLibKernel_stub_weak
  SceKernelThreadMgr_stub_weak
  SceCtrl_stub_weak
  SceIofilemgr_stub_weak
  SceAppMgr_stub_weak
  SceTouch_stub_weak
  SceDisplay_stub_weak
  SceSysmem_stub_weak
  SceMotion_stub 
)

set_target_properties(remaPSV2
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(remaPSV2.suprx remaPSV2
  CONFIG ${CMAKE_SOURCE_DIR}/remaPSV2.yml
)
