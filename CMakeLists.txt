cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#/../ viet the nay de lui len 1 cap, de cmake biet cac folder cung cap voi project hien tai
# Con viet / thi cmake se hieu la folder project hien tai 
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../Test_source/max30102_test/component
                         ${CMAKE_CURRENT_SOURCE_DIR}/../Test_source/inmp441_test/Filter
                         ${CMAKE_CURRENT_LIST_DIR}/Sensor_init)

project(PPG_PCG_ECG_synchro)