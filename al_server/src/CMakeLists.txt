set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})
set(AL_SERVER_VERSION_MAJOR 1)
set(AL_SERVER_VERSION_MINOR 0)



#
### HDF5 package
#
find_package(HDF5 REQUIRED)


if( NOT HDF5_FOUND )
	message(FATAL_ERROR "Package HDF5 not found")
endif( NOT HDF5_FOUND )

include_directories(${HDF5_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${HDF5_LIBRARIES})
set(LIBRARIES ${LIBRARIES} ${HDF5_HL_LIBRARIES})

message(STATUS "HDF5: " ${HDF5_LIBRARIES})
message(STATUS "HDF5: " ${HDF5_HL_LIBRARIES})

#
### OpenCV
#
find_package( OpenCV REQUIRED )

if( NOT OpenCV_FOUND ) 
    message(FATAL_ERROR "Package OpenCV not found")
endif( NOT OpenCV_FOUND )

include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${OpenCV_LIBRARIES})



#
### CBLAS
#
find_package(CBLAS REQUIRED)

if( NOT CBLAS_FOUND )
    message(FATAL_ERROR "Package CBLAS not found")
endif( NOT CBLAS_FOUND )

include_directories(${CBLAS_INCLUDE_DIR})
set(LIBRARIES ${LIBRARIES} ${CBLAS_LIBRARIES})




#
### Jansson
#
find_package(JANSSON REQUIRED)
if( NOT JANSSON_FOUND )
    message(FATAL_ERROR "Package Jansson not found")
endif( NOT JANSSON_FOUND )
  
include_directories(${JANSSON_INCLUDE_DIR})
set(LIBRARIES ${LIBRARIES} ${JANSSON_LIBRARIES})
 
 
 

#
### gengetopt 
#
find_program(GENGETOPT gengetopt)
if( GENGETOPT STREQUAL "GENGETOPT-NOTFOUND")
	message(FATAL_ERROR "Unable to find gengetopt")
endif( GENGETOPT STREQUAL "GENGETOPT-NOTFOUND")





#
## Generate cmdline.c, cmdline.h and base_config.h
# 
configure_file("${PROJECT_SOURCE_DIR}/src/options.ggo.in" "${CMAKE_CURRENT_BINARY_DIR}/options.ggo")
configure_file("${PROJECT_SOURCE_DIR}/src/base_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/base_config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cmdline.c ${CMAKE_CURRENT_BINARY_DIR}/cmdline.h
	COMMAND ${GENGETOPT} 
	ARGS  -i ${CMAKE_CURRENT_BINARY_DIR}/options.ggo
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/options.ggo
)



#
# Executable
#
add_executable(al_server server.cpp learner.cpp data.cpp ocvsvm.cpp classifier.cpp 
                sampler.cpp cmdline.c)
               
add_dependencies(al_server ${CMAKE_CURRENT_BINARY_DIR}/cmdline.c)
 
target_link_libraries(al_server ${LIBRARIES})
set_target_properties(al_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(al_server PROPERTIES OUTPUT_NAME al_serverd${BUILD_POSTFIX})


