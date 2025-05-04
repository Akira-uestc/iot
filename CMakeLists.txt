cmake_minimum_required(VERSION 3.31)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(iot)
add_compile_options(-Wall -Wextra -Wpedantic -Werror -g)

set(CMAKE_CXX_STANDARD 11)

include_directories(src/Inc)
add_executable(iot src/Src/main.cpp
        src/Src/coder.cpp
        src/Src/send.cpp
        src/Src/update.cpp
        src/Src/data.cpp
        src/Src/json_parser.cpp
        src/Src/camera.cpp
        src/Src/receive.cpp
)

find_library(CJSON_LIB cjson)
target_include_directories(iot PRIVATE /usr/include/cjson)
target_link_libraries(iot PRIVATE ${CJSON_LIB})

find_package(OpenCV REQUIRED)
target_include_directories(iot PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(iot PRIVATE ${OpenCV_LIBS})
