cmake_minimum_required(VERSION 3.0.0)
project(caenhv VERSION 0.1.0)

find_package(ROOT REQUIRED)

include(CTest)
enable_testing()

add_executable(hvcontrol hvcontrol.cc)
target_link_libraries(hvcontrol -lncurses -lcaenhvwrapper)

add_executable(hvserver hvserver.cc)
target_include_directories(hvserver PRIVATE ${ROOT_INCLUDE_DIRS})
target_link_libraries(hvserver -lcaenhvwrapper ${ROOT_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
