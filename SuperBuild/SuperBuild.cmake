cmake_minimum_required(VERSION 2.8.4)
include(ExternalProject)

set(base "${CMAKE_BINARY_DIR}/ExternalProjects")
set_property(DIRECTORY PROPERTY EP_BASE ${base})

set(install_dir "${base}/Install")

option(BUILD_SHARED_LIBS "Should Farsight be built with shared libraries? (Not possible on Windows)" OFF)
if(WIN32)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Farsight cannot built with shared libraries on Windows" FORCE)
  mark_as_advanced(BUILD_SHARED_LIBS)
endif()
set(shared ${BUILD_SHARED_LIBS}) # setting to use for BUILD_SHARED_LIBS on all subsequent projects
set(testing OFF) # setting to use for BUILD_TESTING on all subsequent projects

############################################################################
# Boost
#
ExternalProject_Add(Boost
  URL http://farsight-toolkit.org/support/boost_1_47_0.tar.gz
  URL_MD5 ff180a5276bec773a7625cac7e2288e8
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)  
set(boost "${base}/Source/Boost")

# Compute -G arg for configuring external projects with the same CMake generator:
#
if(CMAKE_EXTRA_GENERATOR)
  set(gen "${CMAKE_EXTRA_GENERATOR} - ${CMAKE_GENERATOR}")
else()
  set(gen "${CMAKE_GENERATOR}")
endif()

# Set the default build type---this will affect all libraries and
# applications
#
set(build_type "")
if(CMAKE_BUILD_TYPE)
  set(build_type "${CMAKE_BUILD_TYPE}")
endif()

option(DEBUG_LEAKS "SHOW VTK DEBUG LEAKS" ON)
if (DEBUG_LEAKS)
  set(vtk_debug_leaks ON)
endif()
set(mac_args)
if(APPLE)
  set(mac_args
    -DVTK_USE_CARBON:BOOL=ON
    -DVTK_USE_COCOA:BOOL=OFF
    -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=""
    -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
    -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
    )
  #Mac OS 10.7 (Lion) apparently ships with a version of PNG that VXL doesn't like.
  #If we're building on this OS, force VXL to build its own version of PNG
  if(${CMAKE_SYSTEM} MATCHES "Darwin-11")
    set(png_arg "-DVXL_FORCE_V3P_PNG:BOOL=ON")
  endif()
endif()

############################################################################
# VXL
#
ExternalProject_Add(VXL
  SVN_REPOSITORY "https://vxl.svn.sourceforge.net/svnroot/vxl/trunk"
  SVN_REVISION -r "32878"
  SVN_TRUST_CERT 1
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}/vxl
    -DCMAKE_BUILD_TYPE:STRING=${build_type}
    -DBUILD_SHARED_LIBS:BOOL=${shared}
    -DBUILD_BRL:BOOL=OFF
    -DBUILD_CONVERSIONS:BOOL=OFF
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_GEL:BOOL=OFF
    -DBUILD_OUL:BOOL=OFF
    -DBUILD_OXL:BOOL=OFF
    -DBUILD_PRIP:BOOL=OFF
    -DBUILD_TBL:BOOL=OFF
    -DBUILD_RPL:BOOL=ON
    -DBUILD_RPL_RGTL:BOOL=ON
    -DBUILD_RPL_RTVL:BOOL=ON
    -DBUILD_TESTING:BOOL=${testing}
    ${mac_args}
    ${png_arg}
  INSTALL_COMMAND ""
)
set(VXL_DIR ${base}/Build/VXL)

############################################################################
# Qt
#
find_package(Qt4)

if(QT_QMAKE_EXECUTABLE)
  set(USE_SYSTEM_QT_DEFAULT ON)
else()
  set(USE_SYSTEM_QT_DEFAULT OFF)
endif()

option(USE_SYSTEM_QT "Use the system Qt4" ${USE_SYSTEM_QT_DEFAULT})

set(Qt_Target "")

if(NOT USE_SYSTEM_QT)
  unset(QT_QMAKE_EXECUTABLE CACHE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/BuildQt.cmake)
  ExternalProject_Get_Property(Qt binary_dir)
  set(QT_QMAKE_EXECUTABLE "${binary_dir}/bin/qmake${CMAKE_EXECUTABLE_SUFFIX}")
  set(Qt_Target "Qt")
endif()

############################################################################
# VTK
#
ExternalProject_Add(VTK
  URL http://farsight-toolkit.org/support/VTK-Source-Aug-23-2011.tar.gz
  URL_MD5 05ff74f0c562084ffcfe6cf203355eb7 
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}
    -DCMAKE_BUILD_TYPE:STRING=${build_type}
    -DVTK_DEBUG_LEAKS:BOOL=${vtk_debug_leaks}
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=${shared}
    -DBUILD_TESTING:BOOL=${testing}
    -DDESIRED_QT_VERSION:STRING=4
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    -DVTK_USE_GUISUPPORT:BOOL=ON
    -DVTK_USE_QT:BOOL=ON
    -DVTK_USE_QTCHARTS:BOOL=ON
    -DVTK_USE_RPATH:BOOL=ON
    -DVTK_QT_USE_WEBKIT:BOOL=OFF
    -DBoost_INCLUDE_DIR:FILEPATH=${boost}
    ${mac_args}
  INSTALL_COMMAND ""
  DEPENDS
    ${Qt_Target}
)
set(VTK_DIR ${base}/Build/VTK)

# ITK v4 needs a short path on Windows
# We setup different (shorter) directories for ITK here if necessary
if(WIN32)
  set(ITK_BASE_DIR_DEFAULT "C:/ITKRoot")
  set(ITK_BASE_DIR "${ITK_BASE_DIR_DEFAULT}"
    CACHE PATH "Base of all SuperBuild-built ITK source/build trees.  If this path is too long, ITK will fail to build.")
  set(ITK_DOWNLOAD_DIR "${ITK_BASE_DIR}/Download")
  set(ITK_SOURCE_DIR "${ITK_BASE_DIR}/src")
  set(ITK_BINARY_DIR "${ITK_BASE_DIR}/bin")
else()
  set(ITK_DOWNLOAD_DIR "${base}/Download/ITK")
  set(ITK_SOURCE_DIR "${base}/Source/ITK")
  set(ITK_BINARY_DIR "${base}/Build/ITK")
endif()

ExternalProject_Add(ITK
  URL http://farsight-toolkit.org/support/ITK-Source-Oct-5-2011.tar.gz
  URL_MD5 5b056969356b856fae677e489f0b181d
  DOWNLOAD_DIR ${ITK_DOWNLOAD_DIR}
  SOURCE_DIR ${ITK_SOURCE_DIR}
  BINARY_DIR ${ITK_BINARY_DIR}
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}
    -DCMAKE_BUILD_TYPE:STRING=${build_type}
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_SHARED_LIBS:BOOL=${shared}
    -DBUILD_TESTING:BOOL=${testing}
    -DITK_USE_REVIEW:BOOL=ON
    -DITK_USE_SYSTEM_VXL:BOOL=ON
    -DVXL_DIR:FILEPATH=${base}/Build/VXL
    ${mac_args}
  INSTALL_COMMAND ""
  DEPENDS
    "VXL"
)

#check if we can build vessel
FIND_PACKAGE(GLUT)
IF(GLUT_FOUND)
  SET(BUILD_VESSEL ON CACHE BOOL "Build Vessel Surface Segmentation")
ELSE()
  SET(BUILD_VESSEL OFF CACHE BOOL "Build Vessel Surface Segmentation")
ENDIF()

option(USE_KPLS "Use KPLS module for classification" OFF)

ExternalProject_Add(Farsight
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/Farsight"
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${build_type}
    -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}
    -DBUILD_SHARED_LIBS:BOOL=${shared}
    -DBoost_INCLUDE_DIR:FILEPATH=${boost}
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    -DVTK_DIR:FILEPATH=${VTK_DIR}
    -DITK_DIR:FILEPATH=${ITK_BINARY_DIR}
    -DVXL_DIR:FILEPATH=${VXL_DIR}
    -DBUILD_VESSEL:BOOL=${BUILD_VESSEL}
    -DUSE_KPLS:BOOL=${USE_KPLS}
    ${mac_args}
  DEPENDS
    VXL
    ITK
    VTK
)
