cmake_minimum_required (VERSION 2.8)

project (mpt C)

include_directories(src/include src/mptlib ./nat_conf ${PROJECT_BINARY_DIR}/libssl/include)
link_directories(${PROJECT_BINARY_DIR}/libssl/)
file(GLOB_RECURSE mptsrv_src src/mptsrv/main.c)
file(GLOB_RECURSE mpt_src src/mpt/mpt.c)
file(GLOB_RECURSE mptlib_src src/mptlib/*.c)
file(GLOB_RECURSE iniparser_src nat_conf/iniparser.c)

add_library(mptl STATIC ${mptlib_src})
add_library(iniparser STATIC ${iniparser_src})
add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${PROJECT_BINARY_DIR}/libssl/libcrypto.a)

add_executable(mpt ${mpt_src})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMPT_SERVER")
add_executable(mptsrv ${mptsrv_src})

target_link_libraries(mptsrv mptl iniparser crypto)
target_link_libraries(mpt mptl iniparser crypto)

add_custom_command(TARGET mptl POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy libmptl.a ${PROJECT_BINARY_DIR}/src/mptlib)
add_custom_command(TARGET mptsrv POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove libiniparser.a)
add_custom_command(TARGET mptsrv POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove libmptl.a)

