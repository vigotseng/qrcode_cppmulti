project(make_qr_label)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} libqrencode.so)
target_link_libraries(${PROJECT_NAME} pthread)

