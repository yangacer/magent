cmake_minimum_required (VERSION 2.8)
project ( magent )

include ( contrib/agent/cmake_mod/use_c++11.cmake )
find_package( Boost REQUIRED program_options thread system)

include_directories ( contrib/json
                    contrib/agent
                    ${Boost_INCLUDE_DIRS})

add_subdirectory ( contrib/json )
add_subdirectory ( contrib/agent )
add_subdirectory ( tests )

add_library ( tube_head_getter_ext SHARED 
              tube_head_getter.cpp tube_utils.cpp )
target_link_libraries ( tube_head_getter_ext json agent ${Boost_LIBRARIES}
                        pthread)

add_library ( magent  extloader.cpp )
target_link_libraries ( magent dl json agent ${Boost_LIBRARIES} pthread)
add_executable ( mage main.cpp )
target_link_libraries ( mage magent)
