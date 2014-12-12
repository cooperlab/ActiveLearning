set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})
set(AL_SERVER_VERSION_MAJOR 0)
set(AL_SERVER_VERSION_MINOR 7)



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
## libconfig++
#
find_package(LIBCONFIG REQUIRED)
if( NOT LIBCONFIG_FOUND )
  message(FATAL_ERROR "Package libconfig not found")
endif( NOT LIBCONFIG_FOUND )

include_directories(${LIBCONFIG_INCLUDE_DIR})
set(LIBRARIES ${LIBRARIES} ${LIBCONFIG_LIBRARIES})




#
## Generate cmdline.c, cmdline.h and base_config.h
# 
configure_file("${PROJECT_SOURCE_DIR}/src/base_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/base_config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})


#
# Executable
#
add_executable(al_server server.cpp learner.cpp data.cpp ocvsvm.cpp classifier.cpp 
                sampler.cpp logger.cpp)
               
target_link_libraries(al_server ${LIBRARIES})
set_target_properties(al_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(al_server PROPERTIES OUTPUT_NAME al_serverd${BUILD_POSTFIX})


