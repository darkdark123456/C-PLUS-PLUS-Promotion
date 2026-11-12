# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "RelWithDebInfo")
  file(REMOVE_RECURSE
  "CMakeFiles\\so_arm101_sim_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\so_arm101_sim_autogen.dir\\ParseCache.txt"
  "so_arm101_sim_autogen"
  )
endif()
