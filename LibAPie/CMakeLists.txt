message(STATUS "LibApie")
message(STATUS "LibApie CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "LibApie PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

# Set lists to empty beforehand as to not accidentally take values from parent
set(sources)
set(cxx-sources)
set(apie-includes)
set(apie-link)

set(apie-includes
    /usr/local/include
    /usr/include/mysql
    /usr/local/protobuf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/astar
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/cpp_redis/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/cpp_redis/tacopie/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/http-parser
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/JPSPlusGoalBounding
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/tinyxml2
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/influxdb-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../PBMsg
    ${CMAKE_CURRENT_SOURCE_DIR}/../LibAPie)

set(apie-link
    /usr/lib64/mysql
    /usr/local/lib64
    /usr/local/lib
    /usr/local/protobuf/lib
    /usr/local/apie/lib)

include_directories(${apie-includes})
link_directories(${apie-link})


set(cxx-sources
    PBMsg/common.pb.cc
    PBMsg/mysql_proxy_msg.pb.cc
    PBMsg/opcodes.pb.cc
    PBMsg/pubsub.pb.cc
    PBMsg/route_register.pb.cc
    PBMsg/rpc_msg.pb.cc
    PBMsg/service_discovery.pb.cc
    ThirdParty/http-parser/http_parser.c
    ThirdParty/JPSPlusGoalBounding/BucketPriorityQueue.cpp
    ThirdParty/JPSPlusGoalBounding/DijkstraFloodfill.cpp
    ThirdParty/JPSPlusGoalBounding/Entry.cpp
    ThirdParty/JPSPlusGoalBounding/FastStack.cpp
    ThirdParty/JPSPlusGoalBounding/FPUtil.cpp
    ThirdParty/JPSPlusGoalBounding/GenericHeap.cpp
    ThirdParty/JPSPlusGoalBounding/JPSPlus.cpp
    ThirdParty/JPSPlusGoalBounding/Map.cpp
    ThirdParty/JPSPlusGoalBounding/PrecomputeMap.cpp
    ThirdParty/JPSPlusGoalBounding/ScenarioLoader.cpp
    ThirdParty/JPSPlusGoalBounding/SimpleUnsortedPriorityQueue.cpp
    ThirdParty/JPSPlusGoalBounding/UnsortedPriorityQueue.cpp
    ThirdParty/jsoncpp/src/lib_json/json_writer.cpp
    ThirdParty/jsoncpp/src/lib_json/json_reader.cpp
    ThirdParty/jsoncpp/src/lib_json/json_value.cpp
    ThirdParty/tinyxml2/tinyxml2.cpp
    LibAPie/api/api_impl.cc
    LibAPie/api/forward_handler.cc
    LibAPie/api/hook.cc
    LibAPie/api/logiccmd_handler.cc
    LibAPie/api/os_sys_calls.cc
    LibAPie/api/pb_handler.cc
    LibAPie/api/pubsub.cc
    LibAPie/common/exception_trap.cpp
    LibAPie/common/file.cc
    LibAPie/common/graphics_utility.cpp
    LibAPie/common/string_utils.cpp
    LibAPie/common/utility.cc
    LibAPie/compressor/lz4_compressor_impl.cc
    LibAPie/crypto/crypto_utility.cc
    LibAPie/decompressor/lz4_decompressor_impl.cc
    LibAPie/event/dispatched_thread.cc
    LibAPie/event/dispatcher_impl.cc
    LibAPie/event/event_impl_base.cc
    LibAPie/event/file_event_impl.cc
    LibAPie/event/libevent.cc
    LibAPie/event/libevent_scheduler.cc
    LibAPie/event/real_time_system.cc
    LibAPie/event/signal_impl.cc
    LibAPie/event/timer_impl.cc
    LibAPie/filesystem/directory.cpp
    LibAPie/http/http_parser_impl.cpp
    LibAPie/http/http_parser_settings.cpp
    LibAPie/http/http_request.cpp
    LibAPie/http/http_request_decoder.cpp
    LibAPie/http/http_response.cpp
    LibAPie/http/http_response_decoder.cpp
    LibAPie/http/http_util.cpp
    LibAPie/http/string_util.cpp
    LibAPie/mysql_driver/dao_factory.cpp
    LibAPie/mysql_driver/mysql_connector.cpp
    LibAPie/mysql_driver/mysql_field.cpp
    LibAPie/mysql_driver/mysql_orm.cpp
    LibAPie/mysql_driver/mysql_table.cpp
    LibAPie/mysql_driver/result_set.cpp
    LibAPie/network/address.cc
    LibAPie/network/client_connection.cpp
    LibAPie/network/client_proxy.cpp
    LibAPie/network/command.cpp
    LibAPie/network/ctx.cpp
    LibAPie/network/end_point.cpp
    LibAPie/network/listener_impl.cc
    LibAPie/network/logger.cpp
    LibAPie/network/object.cpp
    LibAPie/network/output_stream.cc
    LibAPie/network/server_connection.cpp
    LibAPie/network/signaler.cpp
    LibAPie/redis_driver/redis_client.cpp
    LibAPie/rpc/client/rpc_client.cpp
    LibAPie/rpc/server/rpc_server.cpp
    LibAPie/rpc/init.cpp
    LibAPie/serialization/byte_buffer.cpp
    LibAPie/serialization/protocol_head.cpp)


# -----------------------------------------------------------------------------
# source generators

foreach(source ${cxx-sources})
  list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/../${source})
endforeach()


add_library(apie SHARED ${sources} ${public_headers})

target_link_libraries(apie -lmysqlclient -levent -levent_pthreads -lpthread -ldl -lc -lrt -lcrypto -lssl -lcpp_redis -ltacopie)