message(STATUS "gateway_server")
message(STATUS "gateway_server CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "gateway_server PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

message(STATUS "gateway_server CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# Set lists to empty beforehand as to not accidentally take values from parent
set(public_headers)
set(sources)
set(cxx-sources)
set(cxx-includes)
set(cxx-link)
set(pb_sources)

set(cxx-includes
    /usr/local/include
    /usr/include/mysql
    /usr/local/protobuf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/astar
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/cpp_redis/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/cpp_redis/tacopie/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/http-parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/tinyxml2
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/influxdb-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../pb_msg
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libapie
    ${CMAKE_CURRENT_SOURCE_DIR}/../common)

set(cxx-link
    /usr/lib64/mysql
    /usr/local/lib64
    /usr/local/lib
    /usr/local/protobuf/lib
    ${PROJECT_BINARY_DIR}/lib
    /usr/local/apie/lib)


include_directories(${cxx-includes})
link_directories(${cxx-link})


set(pb_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../pb_msg/business/login_msg.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../pb_msg/business/rpc_login.pb.cc)

set(cxx-sources
    gateway_server/main.cpp
    gateway_server/service_init.cpp
    gateway_server/logic/gateway_mgr.cpp
    gateway_server/logic/gateway_role.cpp)


# -----------------------------------------------------------------------------
# source generators

foreach(source ${cxx-sources})
  list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/../${source})
endforeach()

foreach(source ${pb_sources})
  list(APPEND sources ${source})
endforeach()

add_executable(gateway_server ${sources} ${public_headers})

target_link_libraries(gateway_server -lmysqlclient -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -llz4 -lcpp_redis -ltacopie -lapie -lprotobuf -lyaml-cpp)