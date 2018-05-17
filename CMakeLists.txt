project(lt)
cmake_minimum_required(VERSION 2.6) 

#header dir
include_directories(lott sys)

#source dir
aux_source_directory(. CFILES)
aux_source_directory(sys CFILES)
aux_source_directory(lott CFILES)

#link_directories
#link_libraries(c rt)
#add_definitions(-DHELLOWORLD_LIBRARY)

add_executable(lt ${CFILES})
target_link_libraries(lt c rt)

