# Install script for directory: /home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample1/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample2/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample3/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample4/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample5/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample6/cmake_install.cmake")
  include("/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/docsample7/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/vitaly/catkin_ws/LTK_CPP_Linux_10_36_0_1/libltkcpp-linux/examples/output/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
