cmake_minimum_required (VERSION 2.8)
project ( magent )

find_package( Boost REQUIRED )

include_directories ( contrib/json
                    contrib/agent
                    ${Boost_INCLUDE_DIRS})

add_subdirectory ( contrib/json )
add_subdirectory ( contrib/agent )
