
set ( test_targets 
      tube_head_getter)

include_directories ( ${PROJECT_SOURCE_DIR} 
                      ${PROJECT_SOURCE_DIR}/contrib/json )

foreach ( t IN LISTS test_targets )
  add_executable ( ${t} ${t}.cpp )
  target_link_libraries ( ${t} ${t}_ext agent json magent dl)
  add_test ( ${t}_test ${t})
endforeach()

add_executable ( extloader extloader.cpp )
target_link_libraries ( extloader magent )
