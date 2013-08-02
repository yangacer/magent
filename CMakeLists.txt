cmake_minimum_required (VERSION 2.8)
project ( magent )

include ( contrib/agent/cmake_mod/use_c++11.cmake )
find_package( Boost REQUIRED )

include_directories ( contrib/json
                    contrib/agent
                    ${Boost_INCLUDE_DIRS})

add_subdirectory ( contrib/json )
add_subdirectory ( contrib/agent )

add_library ( magent tube_head_getter.cpp tube_utils.cpp )
