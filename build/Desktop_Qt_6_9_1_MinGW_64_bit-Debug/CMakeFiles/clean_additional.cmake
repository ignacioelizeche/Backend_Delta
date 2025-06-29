# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\RestApiServer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RestApiServer_autogen.dir\\ParseCache.txt"
  "RestApiServer_autogen"
  )
endif()
