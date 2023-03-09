cmake_minimum_required(VERSION 3.0.0)
project(hvcontrol VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(hvcontrol hvcontrol.cc)
target_link_libraries(hvcontrol -lncurses -lcaenhvwrapper)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
