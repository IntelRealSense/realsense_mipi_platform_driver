cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

add_compile_options(-Wall)

# rs_viewer app
add_executable(rs_viewer
	       v4l2_ds5_mipi.cpp
	       camera_sub_system/CameraCapabilities.cpp
	       camera_sub_system/Stream.cpp
	       gui/StreamView.cpp
	       utils/RealsenseAutoLogger.cpp
	       utils/RealsenseLogger.cpp
	       utils/ScopedFileDescriptor.cpp
	       utils/StreamUtils.cpp)
include_directories(camera_sub_system/include
                    gui/include
                    utils/include
                   /usr/include/opencv4)
set_property(TARGET rs_viewer PROPERTY CXX_STANDARD 14)
target_link_libraries(rs_viewer pthread opencv_core opencv_imgproc opencv_highgui)

# ds5_fw_logger app
add_executable(rs_fw_logger ds5_fw_logger.cpp)
set_property(TARGET rs_fw_logger PROPERTY CXX_STANDARD 14)
target_link_libraries(rs_fw_logger pthread)

# rs_hwmc app
add_executable(rs_hwmc
	       hwmc.cpp
	       utils/RealsenseAutoLogger.cpp
	       utils/RealsenseLogger.cpp
	       utils/ScopedFileDescriptor.cpp
	       utils/StreamUtils.cpp)
include_directories(utils/include)
set_property(TARGET rs_hwmc PROPERTY CXX_STANDARD 14)
target_link_libraries(rs_hwmc pthread)
