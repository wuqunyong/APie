message(STATUS "LibApie")
message(STATUS "LibApie CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "LibApie PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

message(STATUS "LibApie CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/astar
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cpp_redis/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cpp_redis/tacopie/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/http-parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/JPSPlusGoalBounding
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tinyxml2
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/influxdb-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../pb_msg
    ${CMAKE_CURRENT_SOURCE_DIR}/../libapie)

set(cxx-link
    /usr/lib64/mysql
    /usr/local/lib64
    /usr/local/lib
    /usr/local/protobuf/lib
    /usr/local/apie/lib)

include_directories(${cxx-includes})
link_directories(${cxx-link})


set(cxx-sources
    pb_msg/core/common.pb.cc
    pb_msg/core/mysql_proxy_msg.pb.cc
    pb_msg/core/nats_msg.pb.cc
    pb_msg/core/opcodes.pb.cc
    pb_msg/core/pubsub.pb.cc
    pb_msg/core/route_register.pb.cc
    pb_msg/core/rpc_msg.pb.cc
    pb_msg/core/service_discovery.pb.cc
    third_party/http-parser/http_parser.c
    third_party/JPSPlusGoalBounding/BucketPriorityQueue.cpp
    third_party/JPSPlusGoalBounding/DijkstraFloodfill.cpp
    third_party/JPSPlusGoalBounding/Entry.cpp
    third_party/JPSPlusGoalBounding/FastStack.cpp
    third_party/JPSPlusGoalBounding/FPUtil.cpp
    third_party/JPSPlusGoalBounding/GenericHeap.cpp
    third_party/JPSPlusGoalBounding/JPSPlus.cpp
    third_party/JPSPlusGoalBounding/Map.cpp
    third_party/JPSPlusGoalBounding/PrecomputeMap.cpp
    third_party/JPSPlusGoalBounding/ScenarioLoader.cpp
    third_party/JPSPlusGoalBounding/SimpleUnsortedPriorityQueue.cpp
    third_party/JPSPlusGoalBounding/UnsortedPriorityQueue.cpp
    third_party/jsoncpp/src/lib_json/json_writer.cpp
    third_party/jsoncpp/src/lib_json/json_reader.cpp
    third_party/jsoncpp/src/lib_json/json_value.cpp
    third_party/tinyxml2/tinyxml2.cpp
    libapie/api/api_impl.cc
    libapie/api/forward_handler.cc
    libapie/api/hook.cc
    libapie/api/logiccmd_handler.cc
    libapie/api/os_sys_calls.cc
    libapie/api/pb_handler.cc
    libapie/api/pubsub.cc
    libapie/common/exception_trap.cpp
    libapie/common/file.cc
    libapie/common/graphics_utility.cpp
    libapie/common/string_utils.cpp
    libapie/common/utility.cc
    libapie/compressor/lz4_compressor_impl.cc
    libapie/crypto/crypto_utility.cc
    libapie/decompressor/lz4_decompressor_impl.cc
    libapie/event/dispatched_thread.cc
    libapie/event/dispatcher_impl.cc
    libapie/event/event_impl_base.cc
    libapie/event/file_event_impl.cc
    libapie/event/libevent.cc
    libapie/event/libevent_scheduler.cc
    libapie/event/nats_proxy.cc
    libapie/event/real_time_system.cc
    libapie/event/signal_impl.cc
    libapie/event/timer_impl.cc
    libapie/filesystem/directory.cpp
    libapie/http/http_parser_impl.cpp
    libapie/http/http_parser_settings.cpp
    libapie/http/http_request.cpp
    libapie/http/http_request_decoder.cpp
    libapie/http/http_response.cpp
    libapie/http/http_response_decoder.cpp
    libapie/http/http_util.cpp
    libapie/http/string_util.cpp
    libapie/mysql_driver/dao_factory.cpp
    libapie/mysql_driver/mysql_connector.cpp
    libapie/mysql_driver/mysql_field.cpp
    libapie/mysql_driver/mysql_orm.cpp
    libapie/mysql_driver/mysql_table.cpp
    libapie/mysql_driver/result_set.cpp
    libapie/network/address.cc
    libapie/network/client_connection.cpp
    libapie/network/client_proxy.cpp
    libapie/network/command.cpp
    libapie/network/ctx.cpp
    libapie/network/end_point.cpp
    libapie/network/listener_impl.cc
    libapie/network/logger.cpp
    libapie/network/object.cpp
    libapie/network/output_stream.cc
    libapie/network/server_connection.cpp
    libapie/network/signaler.cpp
    libapie/redis_driver/redis_client.cpp
    libapie/rpc/client/rpc_client.cpp
    libapie/rpc/server/rpc_server.cpp
    libapie/rpc/init.cpp
    libapie/serialization/byte_buffer.cpp
    libapie/serialization/protocol_head.cpp)


# -----------------------------------------------------------------------------
# source generators

foreach(source ${cxx-sources})
  list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/../${source})
endforeach()


add_library(apie SHARED ${sources} ${public_headers})

set_target_properties(apie PROPERTIES VERSION "1.0.0" SOVERSION "1")

target_link_libraries(apie -lmysqlclient -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -lcpp_redis -ltacopie -lnats)

message(STATUS "LibApie PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

#add_custom_command(TARGET apie 
#                POST_BUILD
#                COMMAND ${CMAKE_COMMAND} -E echo compile finish
#                COMMAND ${CMAKE_COMMAND} -E make_directory /usr/local/apie/lib/
#                COMMAND ${CMAKE_COMMAND} -E copy apie /usr/local/apie/lib/.
#                VERBATIM
#               )
          