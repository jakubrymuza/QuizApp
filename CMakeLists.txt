cmake_minimum_required(VERSION 3.10)

project(quiz)

add_executable(quiz main.cpp)
install(TARGETS quiz DESTINATION bin)
target_link_libraries(quiz PUBLIC
    gpiod
)
