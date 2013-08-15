cmake_minimum_required (VERSION 2.8)
project ( magent )

option ( MAGENT_ENABLE_PROFILE Off )

include ( contrib/agent/cmake_mod/use_c++11.cmake )
list ( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_mod )

find_package( Boost REQUIRED program_options thread system filesystem)

include_directories ( contrib/json
                      contrib/agent
                      ${Boost_INCLUDE_DIRS}
                      ${PROJECT_SOURCE_DIR})

add_subdirectory ( contrib/json )
add_subdirectory ( contrib/agent )
add_subdirectory ( tests )

add_library ( magent  extloader.cpp chunk_pool.cpp )
target_link_libraries ( magent dl json agent ${Boost_LIBRARIES} pthread )
add_executable ( mage main.cpp )
target_link_libraries ( mage magent )

file (GLOB tube_ext_src extension/tube/*.cpp )
add_library ( tube SHARED ${tube_ext_src} )
target_link_libraries ( tube json agent ${Boost_LIBRARIES} pthread )

