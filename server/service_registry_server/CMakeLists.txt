message(STATUS "ServiceRegistry")
message(STATUS "ServiceRegistry CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "ServiceRegistry PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

message(STATUS "ServiceRegistry CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# Set lists to empty beforehand as to not accidentally take values from parent
set(public_headers)
set(sources)
set(cxx-sources)
set(cxx-includes)
set(cxx-link)

set(cxx-includes
    /usr/local/include
    /usr/include/mysql
    /usr/local/protobuf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/astar
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/cpp_redis/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/cpp_redis/tacopie/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/http-parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/tinyxml2
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/influxdb-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../PBMsg
    ${CMAKE_CURRENT_SOURCE_DIR}/../LibAPie
    ${CMAKE_CURRENT_SOURCE_DIR}/../SharedDir)

set(cxx-link
    /usr/lib64/mysql
    /usr/local/lib64
    /usr/local/lib
    /usr/local/protobuf/lib
    /usr/local/apie/lib
    ${PROJECT_BINARY_DIR}/lib)


include_directories(${cxx-includes})
link_directories(${cxx-link})


set(cxx-sources
    ServiceRegistry/main.cpp
    ServiceRegistry/service_init.cpp
    ServiceRegistry/logic/service_registry.cpp)


# -----------------------------------------------------------------------------
# source generators

foreach(source ${cxx-sources})
  list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/../${source})
endforeach()


add_executable(service_registry ${sources} ${public_headers})

target_link_libraries(service_registry -lmysqlclient -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -llz4 -lcpp_redis -ltacopie -lapie -lprotobuf -lyaml-cpp)